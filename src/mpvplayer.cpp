#include "mpvplayer.hpp"

#include <mpv/client.h>

#include <QDebug>
#include <QMetaObject>

MpvPlayer::MpvPlayer(QObject* parent)
    : QObject(parent)
{
}

MpvPlayer::~MpvPlayer()
{
    shutdown();
}

void MpvPlayer::wakeupCallback(void* context)
{
    auto* player =
        static_cast<MpvPlayer*>(context);

    if (!player)
        return;

    QMetaObject::invokeMethod(
        player,
        &MpvPlayer::processEvents,
        Qt::QueuedConnection
    );
}

void MpvPlayer::processEvents()
{
    if (!m_mpv)
        return;

    while (true) {

        mpv_event* event =
            mpv_wait_event(m_mpv, 0);

        if (!event)
            break;

        if (event->event_id == MPV_EVENT_NONE)
            break;

        handleEvent(event);
    }
}

void MpvPlayer::handleEvent(mpv_event* event)
{
    if (!event)
        return;

    switch (event->event_id) {

    case MPV_EVENT_PROPERTY_CHANGE:
    {
        auto* property =
            static_cast<mpv_event_property*>(
                event->data
            );

        if (!property)
            break;

        if (!property->name)
            break;

        if (qstrcmp(property->name, "time-pos") == 0) {

            if (property->format == MPV_FORMAT_DOUBLE &&
                property->data) {

                const double position =
                    *static_cast<double*>(
                        property->data
                    );

                emit positionChanged(position);
            }

        } else if (
            qstrcmp(property->name, "duration") == 0
        ) {

            if (property->format == MPV_FORMAT_DOUBLE &&
                property->data) {

                const double duration =
                    *static_cast<double*>(
                        property->data
                    );

                emit durationChanged(duration);
            }

        } else if (
            qstrcmp(property->name, "pause") == 0
        ) {

            if (property->format == MPV_FORMAT_FLAG &&
                property->data) {

                const bool paused =
                    *static_cast<int*>(
                        property->data
                    ) != 0;

                emit pauseChanged(paused);
            }

        } else if (
            qstrcmp(property->name, "mute") == 0
        ) {

            if (property->format == MPV_FORMAT_FLAG &&
                property->data) {

                const bool muted =
                    *static_cast<int*>(
                        property->data
                    ) != 0;

                emit muteChanged(muted);
            }

        } else if (
            qstrcmp(property->name, "paused-for-cache") == 0
        ) {

            if (property->format == MPV_FORMAT_FLAG &&
                property->data) {

                const bool buffering =
                    *static_cast<int*>(
                        property->data
                    ) != 0;

                emit bufferingChanged(buffering);
            }
        }

        break;
    }

    case MPV_EVENT_START_FILE:
        emit playbackStarted();
        break;

    case MPV_EVENT_END_FILE:
    {
        auto* endFile =
            static_cast<mpv_event_end_file*>(
                event->data
            );

        if (endFile) {

            if (endFile->reason == MPV_END_FILE_REASON_ERROR) {

                const QString errorMsg =
                    QString::fromUtf8(
                        mpv_error_string(endFile->error)
                    );

                qWarning()
                    << "[MpvPlayer] Error de reproducción:"
                    << errorMsg;

                emit playbackError(errorMsg);

            } else {

                emit playbackFinished();
            }

        } else {

            emit playbackFinished();
        }

        break;
    }

    case MPV_EVENT_FILE_LOADED:
        emit playbackStarted();
        break;

    case MPV_EVENT_SHUTDOWN:
        break;

    case MPV_EVENT_LOG_MESSAGE:
        break;

    default:
        break;
    }
}

void MpvPlayer::observeProperties()
{
    if (!m_mpv)
        return;

    const char* command[] = {
        "observe_property",
        "1",
        "time-pos",
        nullptr
    };

    mpv_command(
        m_mpv,
        command
    );

    const char* command2[] = {
        "observe_property",
        "2",
        "duration",
        nullptr
    };

    mpv_command(
        m_mpv,
        command2
    );

    const char* command3[] = {
        "observe_property",
        "3",
        "pause",
        nullptr
    };

    mpv_command(
        m_mpv,
        command3
    );

    const char* command4[] = {
        "observe_property",
        "4",
        "demuxer-cache-state",
        nullptr
    };

    mpv_command(
        m_mpv,
        command4
    );

    const char* command5[] = {
        "observe_property",
        "5",
        "paused-for-cache",
        nullptr
    };

    mpv_command(
        m_mpv,
        command5
    );

    const char* command6[] = {
        "observe_property",
        "6",
        "mute",
        nullptr
    };

    mpv_command(
        m_mpv,
        command6
    );
}

bool MpvPlayer::initialize(WId wid)
{
    if (m_mpv) {
        qWarning()
            << "[MpvPlayer] mpv ya está inicializado.";

        return true;
    }

    if (!wid) {
        qWarning()
            << "[MpvPlayer] WId inválido.";

        return false;
    }

    m_mpv = mpv_create();

    if (!m_mpv) {
        qCritical()
            << "[MpvPlayer] No se pudo crear la instancia de mpv.";

        return false;
    }

    mpv_set_option_string(
        m_mpv,
        "terminal",
        "no"
    );

    mpv_set_option_string(
        m_mpv,
        "msg-level",
        "all=warn"
    );

    mpv_set_option_string(
        m_mpv,
        "vo",
        "gpu"
    );

    mpv_set_option_string(
        m_mpv,
        "ytdl",
        "no"
    );

#ifdef Q_OS_WIN

    mpv_set_option_string(
        m_mpv,
        "gpu-context",
        "d3d11"
    );

#else

    mpv_set_option_string(
        m_mpv,
        "gpu-context",
        "auto"
    );

#endif

    mpv_set_option_string(
        m_mpv,
        "network-timeout",
        "10"
    );

    mpv_set_option_string(
        m_mpv,
        "reconnect",
        "yes"
    );

    mpv_set_option_string(
        m_mpv,
        "cache",
        "yes"
    );

    mpv_set_option_string(
        m_mpv,
        "demuxer-max-bytes",
        "32MiB"
    );

    mpv_set_option_string(
        m_mpv,
        "demuxer-readahead-secs",
        "10"
    );

    const QByteArray widString =
        QByteArray::number(
            static_cast<quintptr>(wid)
        );

    int result = mpv_set_option_string(
        m_mpv,
        "wid",
        widString.constData()
    );

    if (result < 0) {
        qCritical()
            << "[MpvPlayer] Error configurando WId:"
            << mpv_error_string(result);

        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;

        return false;
    }

    // ----------------------------------------
    // Wakeup callback
    // ----------------------------------------

    mpv_set_wakeup_callback(
        m_mpv,
        &MpvPlayer::wakeupCallback,
        this
    );

    // ----------------------------------------
    // Inicializar
    // ----------------------------------------

    result = mpv_initialize(m_mpv);

    if (result < 0) {
        qCritical()
            << "[MpvPlayer] Error inicializando mpv:"
            << mpv_error_string(result);

        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;

        return false;
    }

    observeProperties();

    qDebug()
        << "[MpvPlayer] mpv inicializado correctamente.";

    return true;
}

void MpvPlayer::shutdown()
{
    if (!m_mpv)
        return;

    qDebug() << "[MpvPlayer] Cerrando mpv...";

    mpv_terminate_destroy(m_mpv);
    m_mpv = nullptr;
}

bool MpvPlayer::setVideoOutput(WId wid)
{
    if (!m_mpv) {
        qWarning()
            << "[MpvPlayer] mpv no está creado.";

        return false;
    }

    if (!wid) {
        qWarning()
            << "[MpvPlayer] WId inválido.";

        return false;
    }

    const QByteArray widString =
        QByteArray::number(
            static_cast<quintptr>(wid)
        );

    const int result = mpv_set_option_string(
        m_mpv,
        "wid",
        widString.constData()
    );

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] No se pudo establecer WId:"
            << mpv_error_string(result);

        return false;
    }

    return true;
}

void MpvPlayer::play(const QString& url)
{
    if (!m_mpv) {
        qWarning() << "[MpvPlayer] mpv no está inicializado.";
        return;
    }

    if (url.isEmpty()) {
        qWarning() << "[MpvPlayer] URL vacía.";
        return;
    }

    const QByteArray urlData = url.toUtf8();

    const char* command[] = {
        "loadfile",
        urlData.constData(),
        "replace",
        nullptr
    };
    
    const int result = mpv_command(m_mpv, command);

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error al reproducir:"
            << mpv_error_string(result);
    }
}

void MpvPlayer::pause()
{
    if (!m_mpv)
        return;

    const char* command[] = {
        "set",
        "pause",
        "yes",
        nullptr
    };

    const int result = mpv_command(m_mpv, command);

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error al pausar:"
            << mpv_error_string(result);
    }
}

void MpvPlayer::stop()
{
    if (!m_mpv)
        return;

    const char* command[] = {
        "stop",
        nullptr
    };

    const int result = mpv_command(m_mpv, command);

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error al detener:"
            << mpv_error_string(result);
    }
}

void MpvPlayer::resume()
{
    if (!m_mpv)
        return;

    const char* command[] = {
        "set",
        "pause",
        "no",
        nullptr
    };

    const int result = mpv_command(m_mpv, command);

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error al reanudar:"
            << mpv_error_string(result);
    }
}

void MpvPlayer::setMuted(bool muted)
{
    if (!m_mpv)
        return;

    const char* flag = muted ? "yes" : "no";

    const char* command[] = {
        "set",
        "mute",
        flag,
        nullptr
    };

    const int result = mpv_command(m_mpv, command);

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error al establecer el estado de mute:"
            << mpv_error_string(result);
    }
}

bool MpvPlayer::isMuted() const
{
    if (!m_mpv)
        return false;

    int flag = 0;

    const int result = mpv_get_property(
        m_mpv,
        "mute",
        MPV_FORMAT_FLAG,
        &flag
    );

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error obteniendo estado de mute:"
            << mpv_error_string(result);

        return false;
    }

    return flag != 0;
}

void MpvPlayer::setVolume(double volume)
{
    if (!m_mpv)
        return;

    volume = qBound(0.0, volume, 100.0);

    const QByteArray value =
        QByteArray::number(volume, 'f', 2);

    const char* command[] = {
        "set",
        "volume",
        value.constData(),
        nullptr
    };

    const int result = mpv_command(m_mpv, command);

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error al establecer volumen:"
            << mpv_error_string(result);
    }
}

double MpvPlayer::volume() const
{
    if (!m_mpv)
        return 0.0;

    double value = 0.0;

    const int result = mpv_get_property(
        m_mpv,
        "volume",
        MPV_FORMAT_DOUBLE,
        &value
    );

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error obteniendo volumen:"
            << mpv_error_string(result);

        return 0.0;
    }

    return value;
}

void MpvPlayer::seek(double seconds)
{
    if (!m_mpv)
        return;

    const QByteArray value =
        QByteArray::number(seconds, 'f', 3);

    const char* command[] = {
        "seek",
        value.constData(),
        "absolute",
        nullptr
    };

    const int result = mpv_command(m_mpv, command);

    if (result < 0) {
        qWarning()
            << "[MpvPlayer] Error al hacer seek:"
            << mpv_error_string(result);
    }
}

double MpvPlayer::position() const
{
    if (!m_mpv)
        return 0.0;

    double value = 0.0;

    const int result = mpv_get_property(
        m_mpv,
        "time-pos",
        MPV_FORMAT_DOUBLE,
        &value
    );

    if (result < 0)
        return 0.0;

    return value;
}

double MpvPlayer::duration() const
{
    if (!m_mpv)
        return 0.0;

    double value = 0.0;

    const int result = mpv_get_property(
        m_mpv,
        "duration",
        MPV_FORMAT_DOUBLE,
        &value
    );

    if (result < 0)
        return 0.0;

    return value;
}

bool MpvPlayer::isLive() const
{
    if (!m_mpv)
        return false;

    return duration() <= 0.0;
}