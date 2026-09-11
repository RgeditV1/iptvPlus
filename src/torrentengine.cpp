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
#include <QFileInfo>
#include <QHostAddress>
#include <QRegularExpression>

#include <algorithm>
#include <limits>

namespace {

bool isVideoExtension(const QString& path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    static const QStringList extensions = {
        "mkv", "mp4", "m4v", "avi", "webm", "mov", "wmv",
        "mpg", "mpeg", "ts",  "m2ts", "mts",  "flv"
    };
    return extensions.contains(ext);
}

bool isIgnoredFile(const QString& path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    static const QStringList ignored = {
        "txt", "nfo", "jpg", "jpeg", "png", "gif", "srt",
        "ass", "ssa", "sub", "sfv",  "md5", "crc", "url", "lnk"
    };
    return ignored.contains(ext);
}

} // namespace

TorrentEngine::TorrentEngine(QObject* parent)
    : QObject(parent)
{
    qDebug() << "[TorrentEngine] Inicializando libtorrent...";

    libtorrent::settings_pack pack;
    pack.set_str(libtorrent::settings_pack::listen_interfaces, "0.0.0.0:0");

    // Habilitar extensiones de descubrimiento
    pack.set_bool(libtorrent::settings_pack::enable_dht, true);
    pack.set_bool(libtorrent::settings_pack::enable_lsd, true);
    pack.set_bool(libtorrent::settings_pack::enable_upnp, true);
    pack.set_bool(libtorrent::settings_pack::enable_natpmp, true);

    // Activar soporte para protocolo uTP (mejora rendimiento tras NATs)
    pack.set_bool(libtorrent::settings_pack::enable_incoming_utp, true);
    pack.set_bool(libtorrent::settings_pack::enable_outgoing_utp, true);

    pack.set_int(libtorrent::settings_pack::alert_mask,
                 libtorrent::alert_category::error |
                 libtorrent::alert_category::status |
                 libtorrent::alert_category::storage |
                 libtorrent::alert_category::dht |
                 libtorrent::alert_category::peer |
                 libtorrent::alert_category::tracker);

    pack.set_bool(libtorrent::settings_pack::enable_dht, true);
    pack.set_bool(libtorrent::settings_pack::enable_lsd, true);
    pack.set_bool(libtorrent::settings_pack::enable_upnp, true);
    pack.set_bool(libtorrent::settings_pack::enable_natpmp, true);

    pack.set_str(libtorrent::settings_pack::dht_bootstrap_nodes,
                 "router.bittorrent.com:6881,"
                 "router.utorrent.com:6881,"
                 "dht.transmissionbt.com:6881,"
                 "dht.aelitis.com:6881");

    m_session.apply_settings(pack);

    m_httpServer = new QTcpServer(this);
    connect(m_httpServer, &QTcpServer::newConnection, this, &TorrentEngine::onNewHttpConnection);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &TorrentEngine::updateEngineState);

    m_streamTimer = new QTimer(this);
    m_streamTimer->setInterval(25); // No bloquea el event loop
    connect(m_streamTimer, &QTimer::timeout, this, &TorrentEngine::sendNextStreamChunk);

    qDebug() << "[TorrentEngine] Motor libtorrent listo.";
}

TorrentEngine::~TorrentEngine()
{
    qDebug() << "[TorrentEngine] Destruyendo motor...";
    stop();
}

bool TorrentEngine::startMagnet(const QString& magnetUrl, const QString& savePath)
{
    qDebug() << "[TorrentEngine] startMagnet()";
    stop();

    if (magnetUrl.trimmed().isEmpty()) {
        emit errorOccurred("La URL Magnet está vacía.");
        return false;
    }

    if (savePath.trimmed().isEmpty()) {
        emit errorOccurred("La ruta de descarga está vacía.");
        return false;
    }

    QDir dir(savePath);
    if (!dir.exists() && !dir.mkpath(".")) {
        emit errorOccurred(QString("No se pudo crear la carpeta: %1").arg(savePath));
        return false;
    }

    if (!m_httpServer->listen(QHostAddress::LocalHost, 0)) {
        emit errorOccurred(QString("No se pudo iniciar el servidor HTTP: %1").arg(m_httpServer->errorString()));
        return false;
    }

    m_serverPort = m_httpServer->serverPort();
    qDebug() << "[TorrentEngine] HTTP local:" << m_serverPort;

    libtorrent::error_code ec;
    libtorrent::add_torrent_params params = libtorrent::parse_magnet_uri(magnetUrl.toStdString(), ec);

    if (ec) {
        qWarning() << "[TorrentEngine] Error Magnet:" << QString::fromStdString(ec.message());
        emit errorOccurred(QString("Magnet inválido: %1").arg(QString::fromStdString(ec.message())));
        m_httpServer->close();
        return false;
    }

    // --- INYECTAR TRACKERS DE RESPALDO ---
    std::vector<std::string> fallbackTrackers = {
        "udp://tracker.opentrackr.org:1337/announce",
        "udp://open.stealth.si:80/announce",
        "udp://tracker.torrent.eu.org:451/announce",
        "udp://explodie.org:6969/announce"
    };
    params.trackers.insert(params.trackers.end(), fallbackTrackers.begin(), fallbackTrackers.end());

    params.save_path = savePath.toStdString();
    params.flags |= libtorrent::torrent_flags::auto_managed;

    m_handle = m_session.add_torrent(params, ec);

    if (ec) {
        qWarning() << "[TorrentEngine] add_torrent:" << QString::fromStdString(ec.message());
        emit errorOccurred(QString("No se pudo añadir el Magnet: %1").arg(QString::fromStdString(ec.message())));
        m_httpServer->close();
        return false;
    }

    // Streaming secuencial
    m_handle.set_flags(libtorrent::torrent_flags::sequential_download);

    m_videoFileIndex = -1;
    m_fileSize = 0;
    m_fileOffset = 0;

    m_streamStart = 0;
    m_streamEnd = 0;
    m_streamPosition = 0;

    m_isReadyToPlay = false;
    m_streaming = false;
    m_httpHeadersSent = false;
    m_metadataLogged = false;

    m_lastRequestedPiece = libtorrent::piece_index_t(-1);

    // --- CONFIGURAR TIMEOUT (20 segundos sin arrancar) ---
    if (!m_timeoutTimer) {
        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
            if (!m_isReadyToPlay) {
                emit errorOccurred("Tiempo de espera agotado: Sin fuentes suficientes (seeders).");
            }
        });
    }
    m_timeoutTimer->start(180000);

    m_statusTimer->start(500);

    qDebug() << "[TorrentEngine] Torrent añadido.";
    qDebug() << "[TorrentEngine] Esperando metadata...";

    return true;
}

void TorrentEngine::stop()
{
    if (m_statusTimer) m_statusTimer->stop();
    if (m_streamTimer) m_streamTimer->stop();
    if (m_timeoutTimer) m_timeoutTimer->stop();

    stopStreaming();

    if (m_clientSocket) {
        m_clientSocket->disconnect(this);
        m_clientSocket->disconnectFromHost();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    if (m_httpServer) {
        m_httpServer->close();
    }

    if (m_handle.is_valid()) {
        qDebug() << "[TorrentEngine] Eliminando torrent.";
        m_session.remove_torrent(m_handle);
    }

    m_handle = libtorrent::torrent_handle();

    m_videoFileIndex = -1;
    m_fileSize = 0;
    m_fileOffset = 0;

    m_streamStart = 0;
    m_streamEnd = 0;
    m_streamPosition = 0;

    m_serverPort = 0;

    m_isReadyToPlay = false;
    m_streaming = false;
    m_httpHeadersSent = false;
    m_metadataLogged = false;

    m_httpRequestBuffer.clear();
    m_lastRequestedPiece = libtorrent::piece_index_t(-1);

    qDebug() << "[TorrentEngine] Motor detenido.";
}

QString TorrentEngine::streamUrl() const
{
    if (m_serverPort == 0) {
        return {};
    }
    return QString("http://127.0.0.1:%1/stream").arg(m_serverPort);
}

int TorrentEngine::findVideoFileIndex(const libtorrent::torrent_info& info)
{
    const auto& fs = info.layout();
    int bestIndex = -1;
    qint64 bestSize = 0;

    // 1. Buscar archivos con extensión de vídeo
    for (int i = 0; i < info.num_files(); ++i) {
        const libtorrent::file_index_t index(i);
        if (fs.pad_file_at(index)) continue;

        const qint64 size = static_cast<qint64>(fs.file_size(index));
        if (size <= 0) continue;

        const auto nameView = fs.file_name(index);
        const QString name = QString::fromUtf8(nameView.data(), static_cast<int>(nameView.size()));

        if (isIgnoredFile(name)) continue;

        if (isVideoExtension(name) && size > bestSize) {
            bestSize = size;
            bestIndex = i;
        }
    }

    // 2. Si no hay extensión reconocida, tomar el archivo válido más grande
    if (bestIndex < 0) {
        for (int i = 0; i < info.num_files(); ++i) {
            const libtorrent::file_index_t index(i);
            if (fs.pad_file_at(index)) continue;

            const qint64 size = static_cast<qint64>(fs.file_size(index));
            if (size <= 0) continue;

            const auto nameView = fs.file_name(index);
            const QString name = QString::fromUtf8(nameView.data(), static_cast<int>(nameView.size()));

            if (isIgnoredFile(name)) continue;

            if (size > bestSize) {
                bestSize = size;
                bestIndex = i;
            }
        }
    }

    return bestIndex;
}

bool TorrentEngine::initializeVideoFile()
{
    if (!m_handle.is_valid()) return false;

    auto info = m_handle.torrent_file();
    if (!info || !info->is_loaded()) return false;

    m_videoFileIndex = findVideoFileIndex(*info);
    if (m_videoFileIndex < 0) {
        emit errorOccurred("No se encontró ningún archivo de vídeo.");
        return false;
    }

    const auto& fs = info->layout();
    const libtorrent::file_index_t index(m_videoFileIndex);

    m_fileSize = static_cast<qint64>(fs.file_size(index));
    m_fileOffset = static_cast<qint64>(fs.file_offset(index));

    const auto nameView = fs.file_name(index);
    const QString fileName = QString::fromUtf8(nameView.data(), static_cast<int>(nameView.size()));

    qDebug() << "[TorrentEngine] Archivo seleccionado:" << fileName;
    qDebug() << "[TorrentEngine] Tamaño:" << m_fileSize << "bytes";
    qDebug() << "[TorrentEngine] Offset:" << m_fileOffset;

    emit metadataLoaded(fileName, m_fileSize);
    return true;
}

void TorrentEngine::prioritizeStreamingPieces()
{
    if (!m_handle.is_valid()) return;

    auto info = m_handle.torrent_file();
    if (!info || !info->is_loaded() || m_videoFileIndex < 0) return;

    const auto& fs = info->layout();
    const libtorrent::file_index_t fileIndex(m_videoFileIndex);

    const auto first = fs.map_file(fileIndex, 0, 1);
    const auto last = fs.map_file(fileIndex, m_fileSize - 1, 1);

    const auto firstPiece = first.piece;
    const auto lastPiece = last.piece;

    qDebug() << "[TorrentEngine] Rango de vídeo:"
             << static_cast<int>(firstPiece) << "->" << static_cast<int>(lastPiece);

    m_handle.set_sequential_range(firstPiece, lastPiece);

    constexpr int START_PIECES = 8;

    // Prioridad máxima a las primeras piezas
    for (int i = 0; i < START_PIECES; ++i) {
        const auto piece = firstPiece + libtorrent::piece_index_t::diff_type(i);
        if (piece <= lastPiece) {
            m_handle.piece_priority(piece, libtorrent::top_priority);
        }
    }

    // Prioridad alta a las piezas subsecuentes
    for (int i = 0; i < START_PIECES; ++i) {
        const auto piece = firstPiece + libtorrent::piece_index_t::diff_type(START_PIECES + i);
        if (piece <= lastPiece) {
            m_handle.piece_priority(piece, libtorrent::top_priority);
        }
    }
}

bool TorrentEngine::isPieceInsideVideo(libtorrent::piece_index_t piece) const
{
    if (!m_handle.is_valid() || m_videoFileIndex < 0) return false;

    auto info = m_handle.torrent_file();
    if (!info || !info->is_loaded()) return false;

    const auto& fs = info->layout();
    const libtorrent::file_index_t fileIndex(m_videoFileIndex);

    const auto first = fs.map_file(fileIndex, 0, 1);
    const auto last = fs.map_file(fileIndex, m_fileSize - 1, 1);

    return piece >= first.piece && piece <= last.piece;
}

void TorrentEngine::processTorrentAlerts()
{
    std::vector<libtorrent::alert*> alerts;
    m_session.pop_alerts(&alerts);

    for (const libtorrent::alert* alert : alerts) {
        if (auto a = libtorrent::alert_cast<libtorrent::metadata_received_alert>(alert)) {
            Q_UNUSED(a);
            qDebug() << "[TorrentEngine] *** METADATA RECIBIDA ***";
            m_metadataLogged = true;

            if (!initializeVideoFile()) return;
            prioritizeStreamingPieces();
            continue;
        }

        if (auto a = libtorrent::alert_cast<libtorrent::metadata_failed_alert>(alert)) {
            qWarning() << "[TorrentEngine] Metadata FAILED:" << QString::fromStdString(a->error.message());
            emit errorOccurred(QString("No se pudieron obtener los metadatos: %1")
                                   .arg(QString::fromStdString(a->error.message())));
            continue;
        }

        if (auto a = libtorrent::alert_cast<libtorrent::torrent_error_alert>(alert)) {
            qWarning() << "[TorrentEngine] TORRENT ERROR:" << QString::fromStdString(a->error.message());
            continue;
        }

        if (auto a = libtorrent::alert_cast<libtorrent::tracker_error_alert>(alert)) {
            qWarning() << "[TorrentEngine] TRACKER ERROR:" << QString::fromStdString(a->failure_reason());
            continue;
        }

        if (auto a = libtorrent::alert_cast<libtorrent::dht_error_alert>(alert)) {
            qWarning() << "[TorrentEngine] DHT ERROR:" << QString::fromStdString(a->error.message());
            continue;
        }

        if (auto a = libtorrent::alert_cast<libtorrent::read_piece_alert>(alert)) {
            processReadPieceAlert(a);
            continue;
        }

        if (auto a = libtorrent::alert_cast<libtorrent::listen_failed_alert>(alert)) {
            qWarning() << "[TorrentEngine] LISTEN ERROR:" << QString::fromStdString(a->error.message());
            continue;
        }
    }
}

void TorrentEngine::processReadPieceAlert(const libtorrent::read_piece_alert* alert)
{
    if (!alert) return;

    if (alert->error) {
        qWarning() << "[TorrentEngine] read_piece error:" << QString::fromStdString(alert->error.message());
        return;
    }

    qDebug() << "[TorrentEngine] Pieza leída:" << static_cast<int>(alert->piece)
             << "bytes:" << alert->size;
}

void TorrentEngine::updateEngineState()
{
    processTorrentAlerts();

    if (!m_handle.is_valid()) return;

    const auto status = m_handle.status();

    emit progressUpdated(status.progress * 100.0f,
                         status.download_payload_rate,
                         status.num_peers);

    if (!status.has_metadata) {
        qDebug() << QString("[TorrentEngine] Metadata... Peers=%1").arg(status.num_peers);
        return;
    }

    if (m_videoFileIndex < 0) {
        if (!initializeVideoFile()) return;
        prioritizeStreamingPieces();
    }

    if (m_fileSize <= 0) return;

    auto info = m_handle.torrent_file();
    if (!info) return;

    const auto& fs = info->layout();
    const libtorrent::file_index_t fileIndex(m_videoFileIndex);
    const auto first = fs.map_file(fileIndex, 0, 1);

    if (!m_isReadyToPlay && m_handle.have_piece(first.piece)) {
        qDebug() << "[TorrentEngine] Primera pieza disponible. Iniciando reproductor...";
        m_isReadyToPlay = true;
        
        if (m_timeoutTimer) {
            m_timeoutTimer->stop();
        }

        emit readyToPlay();
    }
}

bool TorrentEngine::parseHttpRequest(const QByteArray& request)
{
    if (!request.startsWith("GET /stream")) {
        return false;
    }

    m_streamStart = 0;
    m_streamEnd = m_fileSize - 1;

    const QList<QByteArray> lines = request.split('\n');

    for (QByteArray line : lines) {
        line = line.trimmed();

        if (!line.toLower().startsWith("range:")) continue;

        const int eq = line.indexOf('=');
        if (eq < 0) continue;

        QByteArray range = line.mid(eq + 1).trimmed(); // e.g. "0-" o "100-500"
        const int dash = range.indexOf('-');
        if (dash < 0) continue;

        const QByteArray startBytes = range.left(dash).trimmed();
        const QByteArray endBytes = range.mid(dash + 1).trimmed();

        bool ok = false;

        if (!startBytes.isEmpty()) {
            const qint64 start = startBytes.toLongLong(&ok);
            if (ok && start >= 0 && start < m_fileSize) {
                m_streamStart = start;
            }
        }

        if (!endBytes.isEmpty()) {
            const qint64 end = endBytes.toLongLong(&ok);
            if (ok && end >= m_streamStart && end < m_fileSize) {
                m_streamEnd = end;
            }
        }

        break;
    }

    if (m_streamEnd < m_streamStart) {
        m_streamEnd = m_fileSize - 1;
    }

    m_streamPosition = m_streamStart;
    return true;
}

void TorrentEngine::sendHttpResponse(int statusCode,
                                     const QByteArray& statusText,
                                     const QByteArray& contentType,
                                     qint64 contentLength,
                                     const QByteArray& extraHeaders)
{
    if (!m_clientSocket) return;

    const QByteArray headers =
        QByteArray("HTTP/1.1 ") + QByteArray::number(statusCode) + " " + statusText + "\r\n" +
        "Content-Type: " + contentType + "\r\n" +
        "Content-Length: " + QByteArray::number(contentLength) + "\r\n" +
        "Accept-Ranges: bytes\r\n" +
        "Connection: keep-alive\r\n" +
        extraHeaders + "\r\n";

    m_clientSocket->write(headers);
}

void TorrentEngine::onNewHttpConnection()
{
    if (!m_httpServer) return;

    while (m_httpServer->hasPendingConnections()) {
        QTcpSocket* socket = m_httpServer->nextPendingConnection();
        if (!socket) continue;

        if (m_clientSocket) {
            m_clientSocket->disconnectFromHost();
            m_clientSocket->deleteLater();
        }

        m_clientSocket = socket;
        m_httpRequestBuffer.clear();

        connect(m_clientSocket, &QTcpSocket::readyRead, this, &TorrentEngine::onHttpReadyRead);
        connect(m_clientSocket, &QTcpSocket::disconnected, this, [this]() {
            if (m_clientSocket) {
                m_clientSocket->deleteLater();
                m_clientSocket = nullptr;
            }
            stopStreaming();
        });

        qDebug() << "[TorrentEngine HTTP] Nueva conexión.";
    }
}

void TorrentEngine::onHttpReadyRead()
{
    if (!m_clientSocket) return;

    m_httpRequestBuffer += m_clientSocket->readAll();

    if (!m_httpRequestBuffer.contains("\r\n\r\n")) return;

    const QByteArray request = m_httpRequestBuffer;
    m_httpRequestBuffer.clear();

    qDebug() << "[TorrentEngine HTTP] Request:" << request.left(500);

    if (!parseHttpRequest(request)) {
        sendHttpResponse(404, "Not Found", "text/plain", 0);
        m_clientSocket->disconnectFromHost();
        return;
    }

    if (m_videoFileIndex < 0 || m_fileSize <= 0) {
        sendHttpResponse(503, "Service Unavailable", "text/plain", 0);
        return;
    }

    const QString savePath = QString::fromStdString(m_handle.status().save_path);
    auto info = m_handle.torrent_file();
    if (!info) return;

    const auto& fs = info->layout();
    const libtorrent::file_index_t fileIndex(m_videoFileIndex);

    const QString relativePath = QString::fromUtf8(fs.file_path(fileIndex).data(),
                                                   static_cast<int>(fs.file_path(fileIndex).size()));
    const QString absolutePath = QDir::cleanPath(QDir(savePath).filePath(relativePath));

    if (!m_streamFile.isOpen()) {
        m_streamFile.setFileName(absolutePath);

        if (!m_streamFile.open(QIODevice::ReadOnly)) {
            qWarning() << "[TorrentEngine HTTP] No se pudo abrir:" << absolutePath;
            sendHttpResponse(404, "Not Found", "text/plain", 0);
            return;
        }
    }

    const bool isPartial = (m_streamStart != 0 || m_streamEnd != m_fileSize - 1);
    const qint64 contentLength = m_streamEnd - m_streamStart + 1;

    if (isPartial) {
        const QByteArray extra = QByteArray("Content-Range: bytes ") +
                                 QByteArray::number(m_streamStart) + "-" +
                                 QByteArray::number(m_streamEnd) + "/" +
                                 QByteArray::number(m_fileSize) + "\r\n";

        sendHttpResponse(206, "Partial Content", "video/mp4", contentLength, extra);
    } else {
        sendHttpResponse(200, "OK", "video/mp4", contentLength);
    }

    m_httpHeadersSent = true;
    m_streaming = true;

    if (!m_streamTimer->isActive()) {
        m_streamTimer->start();
    }

    qDebug() << "[TorrentEngine HTTP] Streaming iniciado:"
             << m_streamStart << "->" << m_streamEnd;
}

void TorrentEngine::startStreaming()
{
    if (!m_streamTimer->isActive()) {
        m_streamTimer->start();
    }
}

void TorrentEngine::stopStreaming()
{
    m_streaming = false;
    m_httpHeadersSent = false;

    if (m_streamTimer) m_streamTimer->stop();
    if (m_streamFile.isOpen()) m_streamFile.close();

    m_lastRequestedPiece = libtorrent::piece_index_t(-1);
}

void TorrentEngine::sendNextStreamChunk()
{
    if (!m_streaming || !m_clientSocket || !m_clientSocket->isOpen() || !m_handle.is_valid()) {
        return;
    }

    if (m_streamPosition > m_streamEnd) {
        qDebug() << "[TorrentEngine HTTP] Streaming terminado.";
        m_streamTimer->stop();
        m_streaming = false;
        return;
    }

    // Evitar saturar el buffer de escritura del socket
    if (m_clientSocket->bytesToWrite() > 4 * 1024 * 1024) {
        return;
    }

    auto info = m_handle.torrent_file();
    if (!info) return;

    const auto& fs = info->layout();
    const libtorrent::file_index_t fileIndex(m_videoFileIndex);

    const auto request = fs.map_file(fileIndex, m_streamPosition, 1);
    const auto piece = request.piece;

    // Si la pieza no está lista, le damos prioridad alta y esperamos al siguiente tick
    if (!m_handle.have_piece(piece)) {
        m_handle.piece_priority(piece, libtorrent::top_priority);

        for (int i = 1; i <= 8; ++i) {
            const auto nextPiece = piece + libtorrent::piece_index_t::diff_type(i);
            if (isPieceInsideVideo(nextPiece)) {
                m_handle.piece_priority(nextPiece, libtorrent::top_priority);
            }
        }
        return;
    }

    if (!m_streamFile.isOpen()) return;

    const qint64 torrentPieceOffset = static_cast<int>(piece);
    const qint64 offsetInsidePiece = (m_fileOffset + m_streamPosition) - torrentPieceOffset;
    const qint64 pieceSize = fs.piece_size(piece);
    const qint64 availableInPiece = pieceSize - offsetInsidePiece;

    if (availableInPiece <= 0) {
        qWarning() << "[TorrentEngine HTTP] Offset inválido.";
        return;
    }

    const qint64 remaining = m_streamEnd - m_streamPosition + 1;
    const qint64 amount = std::min<qint64>({availableInPiece, remaining, 1024 * 1024});

    if (!m_streamFile.seek(m_streamPosition)) {
        qWarning() << "[TorrentEngine HTTP] No se pudo hacer seek.";
        return;
    }

    const QByteArray data = m_streamFile.read(amount);
    if (data.isEmpty()) {
        qWarning() << "[TorrentEngine HTTP] Lectura vacía.";
        return;
    }

    m_clientSocket->write(data);
    m_streamPosition += data.size();
}