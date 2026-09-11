#include "torrentengine.hpp"

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <limits>

TorrentEngine::TorrentEngine(QObject* parent)
    : QObject(parent)
{
    lt::settings_pack pack;
    pack.set_int(
        lt::settings_pack::alert_mask,
        lt::alert_category::error |
        lt::alert_category::status |
        lt::alert_category::storage
    );

    pack.set_bool(lt::settings_pack::enable_dht, true);
    pack.set_bool(lt::settings_pack::enable_lsd, true);

    m_session.apply_settings(pack);

    m_httpServer = new QTcpServer(this);
    connect(m_httpServer, &QTcpServer::newConnection, this, &TorrentEngine::onNewHttpConnection);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &TorrentEngine::updateEngineState);
}

TorrentEngine::~TorrentEngine()
{
    stop();
}

bool TorrentEngine::startMagnet(const QString& magnetUrl, const QString& savePath)
{
    stop();

    if (magnetUrl.isEmpty()) {
        emit errorOccurred("La URL Magnet está vacía.");
        return false;
    }

    if (savePath.isEmpty()) {
        emit errorOccurred("La ruta de descarga está vacía.");
        return false;
    }

    QDir dir(savePath);
    if (!dir.exists() && !dir.mkpath(".")) {
        emit errorOccurred(QString("No se pudo crear la carpeta de descarga: %1").arg(savePath));
        return false;
    }

    if (!m_httpServer->listen(QHostAddress::LocalHost, 0)) {
        emit errorOccurred(QString("No se pudo iniciar el servidor HTTP local: %1").arg(m_httpServer->errorString()));
        return false;
    }

    m_serverPort = m_httpServer->serverPort();

    lt::error_code ec;
    lt::add_torrent_params params = lt::parse_magnet_uri(magnetUrl.toStdString(), ec);

    if (ec) {
        emit errorOccurred(QString("Magnet inválido: %1").arg(QString::fromStdString(ec.message())));
        m_httpServer->close();
        return false;
    }

    params.save_path = savePath.toStdString();
    m_handle = m_session.add_torrent(params, ec);

    if (ec) {
        emit errorOccurred(QString("Error al añadir Magnet: %1").arg(QString::fromStdString(ec.message())));
        m_httpServer->close();
        return false;
    }

    /*
     * Activamos descarga secuencial.
     * libtorrent 2.1 mantiene este comportamiento mediante
     * torrent_flags::sequential_download.
     */
    m_handle.set_flags(lt::torrent_flags::sequential_download);

    m_isReadyToPlay = false;
    m_videoFileIndex = -1;
    m_fileSize = 0;
    m_fileOffset = 0;

    m_statusTimer->start(500);

    return true;
}

void TorrentEngine::stop()
{
    if (m_statusTimer) {
        m_statusTimer->stop();
    }

    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    if (m_httpServer && m_httpServer->isListening()) {
        m_httpServer->close();
    }

    if (m_handle.is_valid()) {
        m_session.remove_torrent(m_handle);
    }

    m_handle = lt::torrent_handle();

    m_isReadyToPlay = false;
    m_videoFileIndex = -1;
    m_fileSize = 0;
    m_fileOffset = 0;
    m_serverPort = 0;
}

QString TorrentEngine::streamUrl() const
{
    if (m_serverPort == 0) {
        return {};
    }
    return QString("http://127.0.0.1:%1/stream").arg(m_serverPort);
}

int TorrentEngine::findLargestFileIndex(const lt::torrent_info& info)
{
    int largestIndex = -1;
    qint64 largestSize = 0;

    /*
     * libtorrent 2.1:
     * torrent_info::layout() devuelve el file_storage asociado al torrent.
     */
    const lt::file_storage& fs = info.layout();
    const int fileCount = info.num_files();

    for (int i = 0; i < fileCount; ++i) {
        const lt::file_index_t index(i);
        const qint64 size = static_cast<qint64>(fs.file_size(index));

        // Ignorar archivos vacíos
        if (size <= 0) {
            continue;
        }

        // Ignorar archivos de padding
        if (fs.pad_file_at(index)) {
            continue;
        }

        if (size > largestSize) {
            largestSize = size;
            largestIndex = i;
        }
    }

    return largestIndex;
}

void TorrentEngine::prioritizeSequentialPieces()
{
    if (!m_handle.is_valid()) {
        return;
    }

    const auto info = m_handle.torrent_file();
    if (!info || !info->is_loaded()) {
        return;
    }

    m_videoFileIndex = findLargestFileIndex(*info);

    if (m_videoFileIndex < 0) {
        emit errorOccurred("No se encontró ningún archivo válido en el torrent.");
        return;
    }

    const lt::file_storage& fs = info->layout();
    const lt::file_index_t targetFileIdx(m_videoFileIndex);

    m_fileSize = static_cast<qint64>(fs.file_size(targetFileIdx));
    m_fileOffset = static_cast<qint64>(fs.file_offset(targetFileIdx));

    if (m_fileSize <= 0) {
        emit errorOccurred("El archivo seleccionado tiene tamaño cero.");
        m_videoFileIndex = -1;
        return;
    }

    // Desactivar todos los demás archivos
    for (int i = 0; i < info->num_files(); ++i) {
        const lt::file_index_t fileIndex(i);

        if (i == m_videoFileIndex) {
            m_handle.file_priority(fileIndex, lt::default_priority);
        } else {
            m_handle.file_priority(fileIndex, lt::dont_download);
        }
    }

    // Obtener la primera pieza que contiene datos del archivo de vídeo
    const lt::peer_request firstPiece = info->map_file(targetFileIdx, 0, 1);

    // Obtener la última pieza del archivo
    const lt::peer_request lastPiece = info->map_file(targetFileIdx, m_fileSize - 1, 1);

    /*
     * Dar máxima prioridad a las primeras piezas.
     * Esto permite que el reproductor pueda comenzar antes de descargar todo el archivo.
     */
    constexpr int START_PIECES = 5;

    for (int i = 0; i < START_PIECES; ++i) {
        const auto piece = firstPiece.piece + lt::piece_index_t::diff_type(i);

        if (piece < info->end_piece()) {
            m_handle.piece_priority(piece, lt::top_priority);
        }
    }

    /*
     * También damos prioridad a las últimas piezas.
     * Esto es útil para torrents donde el final del vídeo puede ser necesario posteriormente.
     */
    constexpr int END_PIECES = 3;

    for (int i = END_PIECES - 1; i >= 0; --i) {
        const auto piece = lastPiece.piece - lt::piece_index_t::diff_type(i);

        if (piece >= lt::piece_index_t(0)) {
            m_handle.piece_priority(piece, lt::top_priority);
        }
    }

    /*
     * Obtener el nombre del archivo.
     * file_name() sigue estando disponible en file_storage en libtorrent 2.1.
     */
    const auto fileNameView = fs.file_name(targetFileIdx);
    const QString fileName = QString::fromUtf8(fileNameView.data(), static_cast<int>(fileNameView.size()));

    emit metadataLoaded(fileName, m_fileSize);
}

void TorrentEngine::updateEngineState()
{
    if (!m_handle.is_valid()) {
        return;
    }

    const lt::torrent_status status = m_handle.status();

    // Todavía no tenemos metadata
    if (!status.has_metadata) {
        emit progressUpdated(
            status.progress * 100.0f,
            status.download_payload_rate,
            status.num_peers
        );
        return;
    }

    // Cuando recibimos metadata por primera vez, obtenemos el archivo de vídeo
    if (m_videoFileIndex < 0) {
        prioritizeSequentialPieces();
    }

    emit progressUpdated(
        status.progress * 100.0f,
        status.download_payload_rate,
        status.num_peers
    );

    /*
     * NO utilizamos simplemente: status.progress > 0.02
     * porque progress representa el progreso global del torrent, no necesariamente
     * los primeros bytes del archivo de vídeo.
     *
     * Para streaming necesitamos comprobar que el archivo seleccionado tenga piezas disponibles.
     */
    if (!m_isReadyToPlay && m_videoFileIndex >= 0 && m_fileSize > 0) {
        const lt::torrent_info* info = m_handle.torrent_file().get();
        if (!info) {
            return;
        }

        const lt::file_index_t fileIndex(m_videoFileIndex);
        const lt::peer_request firstPiece = info->map_file(fileIndex, 0, 1);
        const lt::piece_index_t piece = firstPiece.piece;

        if (m_handle.have_piece(piece)) {
            m_isReadyToPlay = true;
            emit readyToPlay();
        }
    }
}

void TorrentEngine::onNewHttpConnection()
{
    if (!m_httpServer) {
        return;
    }

    // Por simplicidad sólo mantenemos una conexión de reproducción activa
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    m_clientSocket = m_httpServer->nextPendingConnection();
    if (!m_clientSocket) {
        return;
    }

    connect(m_clientSocket, &QTcpSocket::readyRead, this, &TorrentEngine::onHttpReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected, m_clientSocket, &QObject::deleteLater);
}

void TorrentEngine::onHttpReadyRead()
{
    if (!m_clientSocket) {
        return;
    }

    const QByteArray requestData = m_clientSocket->readAll();
    if (requestData.isEmpty()) {
        return;
    }

    // Actualmente solamente aceptamos: GET /stream
    if (!requestData.startsWith("GET /stream")) {
        const QByteArray response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";

        m_clientSocket->write(response);
        m_clientSocket->disconnectFromHost();
        return;
    }

    if (m_videoFileIndex < 0 || m_fileSize <= 0) {
        const QByteArray response =
            "HTTP/1.1 503 Service Unavailable\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";

        m_clientSocket->write(response);
        return;
    }

    /*
     * IMPORTANTE:
     * Esta versión mantiene la arquitectura del servidor HTTP actual,
     * pero todavía no realiza lecturas asíncronas desde libtorrent.
     *
     * No debemos fingir que QFile puede leer un archivo incompleto
     * de torrent como si estuviera disponible.
     */
    const QString savePath = QString::fromStdString(m_handle.status().save_path);
    const auto info = m_handle.torrent_file();
    if (!info) {
        return;
    }

    const lt::file_storage& fs = info->layout();
    const lt::file_index_t fileIndex(m_videoFileIndex);
    const QString relativePath = QString::fromStdString(fs.file_path(fileIndex));

    QString absolutePath = QDir(savePath).filePath(relativePath);
    absolutePath = QDir::cleanPath(absolutePath);

    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.exists()) {
        const QByteArray response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";

        m_clientSocket->write(response);
        return;
    }

    /*
     * Si el archivo existe y tiene datos suficientes, se puede servir mediante QFile.
     * Para streaming real de torrents, esta parte deberá evolucionar a lecturas
     * por pieza/bloque y esperar a que libtorrent tenga disponible cada pieza.
     */
    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        const QByteArray response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n";

        m_clientSocket->write(response);
        return;
    }

    const qint64 fileSize = file.size();
    const QByteArray headers =
        QByteArray("HTTP/1.1 200 OK\r\n") +
        "Content-Type: video/mp4\r\n" +
        "Accept-Ranges: bytes\r\n" +
        "Content-Length: " + QByteArray::number(fileSize) + "\r\n" +
        "Connection: keep-alive\r\n" +
        "\r\n";

    m_clientSocket->write(headers);

    // Enviar el archivo en bloques para no cargarlo completo en RAM
    constexpr qint64 CHUNK_SIZE = 1024 * 1024;

    while (!file.atEnd()) {
        const QByteArray chunk = file.read(CHUNK_SIZE);
        if (chunk.isEmpty()) {
            break;
        }

        m_clientSocket->write(chunk);

        // Esperar a que Qt libere parte del buffer
        if (!m_clientSocket->waitForBytesWritten(5000)) {
            break;
        }
    }

    file.close();
}