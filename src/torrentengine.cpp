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

#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

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
    
    // Asignar un rango de puertos fijo para facilitar reglas de Firewall
    pack.set_str(libtorrent::settings_pack::listen_interfaces, "0.0.0.0:6881,127.0.0.1:6881");

    pack.set_int(libtorrent::settings_pack::download_rate_limit, 0); // Ilimitado
    pack.set_int(libtorrent::settings_pack::upload_rate_limit, 0);
    pack.set_int(libtorrent::settings_pack::connections_limit, 500);
    pack.set_int(libtorrent::settings_pack::active_downloads, 10);

    pack.set_int(libtorrent::settings_pack::max_allowed_in_request_queue, 2000);
    pack.set_int(libtorrent::settings_pack::send_buffer_watermark, 3 * 1024 * 1024); // 3 MB Buffer
    pack.set_int(libtorrent::settings_pack::max_out_request_queue, 1500);
    pack.set_int(libtorrent::settings_pack::whole_pieces_threshold, 20);

    // Habilitar descubrimiento
    pack.set_bool(libtorrent::settings_pack::enable_dht, true);
    pack.set_bool(libtorrent::settings_pack::enable_lsd, true);
    pack.set_bool(libtorrent::settings_pack::enable_upnp, true);
    pack.set_bool(libtorrent::settings_pack::enable_natpmp, true);

    // Habilitar protocolo uTP y TCP
    pack.set_bool(libtorrent::settings_pack::enable_incoming_utp, true);
    pack.set_bool(libtorrent::settings_pack::enable_outgoing_utp, true);
    pack.set_bool(libtorrent::settings_pack::enable_incoming_tcp, true);
    pack.set_bool(libtorrent::settings_pack::enable_outgoing_tcp, true);

    // Habilitar encriptación
    pack.set_int(libtorrent::settings_pack::out_enc_policy, libtorrent::settings_pack::pe_enabled);
    pack.set_int(libtorrent::settings_pack::in_enc_policy, libtorrent::settings_pack::pe_enabled);
    pack.set_int(libtorrent::settings_pack::allowed_enc_level, libtorrent::settings_pack::pe_both);

    pack.set_int(libtorrent::settings_pack::alert_mask,
                 libtorrent::alert_category::error |
                 libtorrent::alert_category::status |
                 libtorrent::alert_category::storage |
                 libtorrent::alert_category::peer |
                 libtorrent::alert_category::tracker);

    pack.set_str(libtorrent::settings_pack::dht_bootstrap_nodes,
                 "router.bittorrent.com:6881,"
                 "router.utorrent.com:6881,"
                 "dht.transmissionbt.com:6881,"
                 "dht.aelitis.com:6881");

    m_session.apply_settings(pack);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout,
            this, &TorrentEngine::updateEngineState);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);

    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        if (!m_isReadyToPlay) {
            emit errorOccurred(
                "Tiempo de espera agotado: Sin fuentes suficientes (seeders)."
            );
        }
    });

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

    if (magnetUrl.trimmed().isEmpty() || savePath.trimmed().isEmpty()) {
        emit errorOccurred("Datos de magnet o ruta inválidos.");
        return false;
    }

    QDir dir(savePath);
    if (!dir.exists() && !dir.mkpath(".")) {
        emit errorOccurred(QString("No se pudo crear la carpeta: %1").arg(savePath));
        return false;
    }

    libtorrent::error_code ec;
    libtorrent::add_torrent_params params = libtorrent::parse_magnet_uri(magnetUrl.toStdString(), ec);

    if (ec) {
        emit errorOccurred(QString("Magnet inválido: %1").arg(QString::fromStdString(ec.message())));
        return false;
    }

    // --- INYECTAR TRACKERS HTTP/HTTPS (Superan bloqueos UDP/Firewall) ---
    std::vector<std::string> robustTrackers = {
        "http://tracker.opentrackr.org:1337/announce",
        "https://tracker.tamersil.com:443/announce",
        "http://tracker.openbittorrent.com:80/announce",
        "udp://tracker.opentrackr.org:1337/announce",
        "udp://open.stealth.si:80/announce"
    };

    params.trackers.insert(params.trackers.end(), robustTrackers.begin(), robustTrackers.end());
    params.save_path = savePath.toStdString();

    params.storage_mode = libtorrent::storage_mode_sparse; // Asignación rápida eficiente
    
    // Asegurar que auto_managed y paused NO estén interfiriendo
    params.flags |= libtorrent::torrent_flags::sequential_download;
    params.flags |= libtorrent::torrent_flags::auto_managed;
    params.flags &= ~libtorrent::torrent_flags::paused;

    m_handle = m_session.add_torrent(params, ec);

    if (ec) {
        emit errorOccurred(QString("No se pudo añadir el Magnet: %1").arg(QString::fromStdString(ec.message())));
        m_handle = libtorrent::torrent_handle();
        return false;
    }

    m_videoFilePath.clear();
    m_videoFileIndex = -1;
    m_fileSize = 0;
    m_fileOffset = 0;
    m_isReadyToPlay = false;
    m_metadataLogged = false;

    m_timeoutTimer->start(180000); // 3 minutos
    m_statusTimer->start(500);

    qDebug() << "[TorrentEngine] Torrent añadido con éxito.";
    return true;
}

void TorrentEngine::cleanTempDirectory()
{
    const QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString savePath = QDir(tempPath).filePath("iptv_torrents");
    
    QDir dir(savePath);
    if (dir.exists()) {
        if (dir.removeRecursively()) {
            qDebug() << "[TorrentEngine] Carpeta temporal purgada exitosamente al iniciar:" << savePath;
        } else {
            qWarning() << "[TorrentEngine] No se pudieron eliminar algunos archivos residuales en:" << savePath;
        }
    }
}

void TorrentEngine::stop()
{
    if (m_statusTimer) {
        m_statusTimer->stop();
    }

    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }

    if (m_handle.is_valid()) {
        qDebug() << "[TorrentEngine] Eliminando torrent y borrando archivos descargados de disco...";
        
        // --- ELIMINA LOS ARCHIVOS TEMPORALES Y LA CARPETA EN DISCO ---
        m_session.remove_torrent(m_handle, libtorrent::session_handle::delete_files);
    }

    m_handle = libtorrent::torrent_handle();

    m_videoFilePath.clear();
    m_videoFileIndex = -1;
    m_fileSize = 0;
    m_fileOffset = 0;

    m_isReadyToPlay = false;
    m_metadataLogged = false;

    qDebug() << "[TorrentEngine] Motor detenido y disco limpiado.";
}

QString TorrentEngine::videoFilePath() const
{
    return m_videoFilePath;
}

int TorrentEngine::findVideoFileIndex(
    const libtorrent::torrent_info& info)
{
    const auto& fs = info.layout();

    int bestIndex = -1;
    qint64 bestSize = 0;

    // 1. Buscar archivos con extensión de vídeo
    for (int i = 0; i < info.num_files(); ++i) {
        const libtorrent::file_index_t index(i);

        if (fs.pad_file_at(index)) {
            continue;
        }

        const qint64 size =
            static_cast<qint64>(fs.file_size(index));

        if (size <= 0) {
            continue;
        }

        const auto nameView = fs.file_name(index);

        const QString name =
            QString::fromUtf8(
                nameView.data(),
                static_cast<int>(nameView.size())
            );

        if (isIgnoredFile(name)) {
            continue;
        }

        if (isVideoExtension(name) && size > bestSize) {
            bestSize = size;
            bestIndex = i;
        }
    }

    // 2. Si no hay extensión reconocida,
    //    tomar el archivo válido más grande
    if (bestIndex < 0) {
        for (int i = 0; i < info.num_files(); ++i) {
            const libtorrent::file_index_t index(i);

            if (fs.pad_file_at(index)) {
                continue;
            }

            const qint64 size =
                static_cast<qint64>(fs.file_size(index));

            if (size <= 0) {
                continue;
            }

            const auto nameView = fs.file_name(index);

            const QString name =
                QString::fromUtf8(
                    nameView.data(),
                    static_cast<int>(nameView.size())
                );

            if (isIgnoredFile(name)) {
                continue;
            }

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
    if (!m_handle.is_valid()) {
        return false;
    }

    auto info = m_handle.torrent_file();

    if (!info) {
        return false;
    }

    m_videoFileIndex = findVideoFileIndex(*info);

    if (m_videoFileIndex < 0) {
        emit errorOccurred(
            "No se encontró ningún archivo de vídeo."
        );
        return false;
    }

    const auto& fs = info->layout();

    const libtorrent::file_index_t index(m_videoFileIndex);

    m_fileSize =
        static_cast<qint64>(fs.file_size(index));

    m_fileOffset =
        static_cast<qint64>(fs.file_offset(index));

    const auto nameView = fs.file_name(index);

    const QString fileName =
        QString::fromUtf8(
            nameView.data(),
            static_cast<int>(nameView.size())
        );

    const auto pathView = fs.file_path(index);

    const QString relativePath =
        QString::fromUtf8(
            pathView.data(),
            static_cast<int>(pathView.size())
        );

    const QString savePath =
        QString::fromStdString(
            m_handle.status().save_path
        );

    m_videoFilePath =
        QDir::cleanPath(
            QDir(savePath).filePath(relativePath)
        );

    qDebug() << "[TorrentEngine] Archivo seleccionado:"
             << fileName;

    qDebug() << "[TorrentEngine] Ruta relativa:"
             << relativePath;

    qDebug() << "[TorrentEngine] Ruta absoluta:"
             << m_videoFilePath;

    qDebug() << "[TorrentEngine] Tamaño:"
             << m_fileSize << "bytes";

    qDebug() << "[TorrentEngine] Offset:"
             << m_fileOffset;

    emit metadataLoaded(fileName, m_fileSize);

    return true;
}

bool TorrentEngine::hasInitialVideoPieces() const
{
    if (!m_handle.is_valid() || m_videoFileIndex < 0 || m_fileSize <= 0) 
        return false;

    auto info = m_handle.torrent_file();
    if (!info) return false;

    const auto& fs = info->layout();
    const libtorrent::file_index_t fileIndex(m_videoFileIndex);

    const auto first = fs.map_file(fileIndex, 0, 1).piece;
    
    constexpr int REQUIRED_PIECES = 15;
    for (int i = 0; i < REQUIRED_PIECES; ++i) {
        if (!m_handle.have_piece(first + libtorrent::piece_index_t::diff_type(i))) {
            return false;
        }
    }

    return true;
}

void TorrentEngine::prioritizeStreamingPieces()
{
    if (!m_handle.is_valid()) return;

    auto info = m_handle.torrent_file();
    if (!info || m_videoFileIndex < 0 || m_fileSize <= 0) return;

    // Descargar ÚNICAMENTE el archivo de vídeo seleccionado
    std::vector<libtorrent::download_priority_t> filePriorities(info->num_files(), libtorrent::dont_download);
    filePriorities[m_videoFileIndex] = libtorrent::default_priority;
    m_handle.prioritize_files(filePriorities);

    // Activar modo secuencial estricto en la manija del torrent
    m_handle.set_flags(libtorrent::torrent_flags::sequential_download);

    const auto& fs = info->layout();
    const libtorrent::file_index_t fileIndex(m_videoFileIndex);

    const auto firstPiece = fs.map_file(fileIndex, 0, 1).piece;
    const auto lastPiece = fs.map_file(fileIndex, m_fileSize - 1, 1).piece;

    for (int i = 0; i < 15; ++i) {
        const auto piece = firstPiece + libtorrent::piece_index_t::diff_type(i);
        if (piece <= lastPiece) {
            m_handle.set_piece_deadline(piece, 100 + (i * 50));
        }
    }

    for (int i = 0; i < 5; ++i) {
        const auto piece = lastPiece - libtorrent::piece_index_t::diff_type(i);
        if (piece >= firstPiece) {
            m_handle.set_piece_deadline(piece, 500);
        }
    }

    m_handle.resume();
    qDebug() << "[TorrentEngine] Descarga secuencial y deadlines configurados.";
}

void TorrentEngine::processTorrentAlerts()
{
    std::vector<libtorrent::alert*> alerts;

    m_session.pop_alerts(&alerts);

    for (const libtorrent::alert* alert : alerts) {
        if (auto a =
                libtorrent::alert_cast<
                    libtorrent::metadata_received_alert>(
                    alert)) {

            Q_UNUSED(a);

            qDebug()
                << "[TorrentEngine] *** METADATA RECIBIDA ***";

            m_metadataLogged = true;

            if (!initializeVideoFile()) {
                continue;
            }

            prioritizeStreamingPieces();

            continue;
        }

        if (auto a =
                libtorrent::alert_cast<
                    libtorrent::metadata_failed_alert>(
                    alert)) {

            qWarning()
                << "[TorrentEngine] Metadata FAILED:"
                << QString::fromStdString(
                       a->error.message()
                   );

            emit errorOccurred(
                QString("No se pudieron obtener los metadatos: %1")
                    .arg(
                        QString::fromStdString(
                            a->error.message()
                        )
                    )
            );

            continue;
        }

        if (auto a =
                libtorrent::alert_cast<
                    libtorrent::torrent_error_alert>(
                    alert)) {

            qWarning()
                << "[TorrentEngine] TORRENT ERROR:"
                << QString::fromStdString(
                       a->error.message()
                   );

            emit errorOccurred(
                QString("Error del torrent: %1")
                    .arg(
                        QString::fromStdString(
                            a->error.message()
                        )
                    )
            );

            continue;
        }

        if (auto a =
                libtorrent::alert_cast<
                    libtorrent::tracker_error_alert>(
                    alert)) {

            qWarning()
                << "[TorrentEngine] TRACKER ERROR:"
                << QString::fromStdString(
                       a->failure_reason()
                   );

            continue;
        }

        if (auto a =
                libtorrent::alert_cast<
                    libtorrent::dht_error_alert>(
                    alert)) {

            qWarning()
                << "[TorrentEngine] DHT ERROR:"
                << QString::fromStdString(
                       a->error.message()
                   );

            continue;
        }

        if (auto a =
                libtorrent::alert_cast<
                    libtorrent::listen_failed_alert>(
                    alert)) {

            qWarning()
                << "[TorrentEngine] LISTEN ERROR:"
                << QString::fromStdString(
                       a->error.message()
                   );

            continue;
        }
    }
}

void TorrentEngine::updateEngineState()
{
    processTorrentAlerts();

    if (!m_handle.is_valid()) {
        return;
    }

    const auto status = m_handle.status();

    emit progressUpdated(
        status.progress * 100.0f,
        status.download_payload_rate,
        status.num_peers
    );

    if (!status.has_metadata) {
        qDebug()
            << QString(
                   "[TorrentEngine] Metadata... "
                   "Peers=%1"
               )
                   .arg(status.num_peers);

        return;
    }

    if (m_videoFileIndex < 0) {
        if (!initializeVideoFile()) {
            return;
        }

        prioritizeStreamingPieces();
    }

    if (m_fileSize <= 0) {
        return;
    }

    if (!m_isReadyToPlay &&
        hasInitialVideoPieces()) {

        qDebug()
            << "[TorrentEngine] Primera pieza disponible."
            << "Iniciando reproductor...";

        qDebug()
            << "[TorrentEngine] Archivo:"
            << m_videoFilePath;

        m_isReadyToPlay = true;

        if (m_timeoutTimer) {
            m_timeoutTimer->stop();
        }

        emit readyToPlay();
    }
}