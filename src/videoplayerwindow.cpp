#include "videoplayerwindow.hpp"
#include <QDebug>

VideoPlayerWindow::VideoPlayerWindow(QWidget* parent)
    : QWidget(parent), mpv(nullptr), mpvTimer(nullptr)
{
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setStyleSheet("background-color: black;");

    initMpv();
}

VideoPlayerWindow::~VideoPlayerWindow() {
    if (mpv) {
        mpv_terminate_destroy(mpv);
    }
}

void VideoPlayerWindow::initMpv() {
    mpv = mpv_create();
    if (!mpv) {
        qCritical() << "[VideoPlayerWindow] No se pudo crear la instancia de mpv.";
        return;
    }

    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "msg-level", "all=v");
    mpv_set_option_string(mpv, "vo", "gpu");
    mpv_set_option_string(mpv, "gpu-context", "d3d11");
    mpv_set_option_string(mpv, "user-agent", "Mozilla/5.0");
    mpv_set_option_string(mpv, "tls-verify", "no");

    int64_t wid = static_cast<int64_t>(winId());
    mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);

    if (mpv_initialize(mpv) < 0) {
        qCritical() << "[VideoPlayerWindow] No se pudo inicializar mpv.";
        return;
    }

    // Solicitar logs internos de mpv
    mpv_request_log_messages(mpv, "info");

    // Procesar periódicamente los eventos de mpv
    mpvTimer = new QTimer(this);

    connect(mpvTimer, &QTimer::timeout,
        this, &VideoPlayerWindow::onMpvEvents);

    mpvTimer->start(10);

    qDebug() << "[VideoPlayerWindow] Instancia MPV inicializada correctamente.";
}

void VideoPlayerWindow::playMedia(const QString& url) {
    if (!mpv) {
        qWarning() << "[VideoPlayerWindow] No se puede reproducir: la instancia MPV no existe.";
        return;
    }

    qDebug() << "[VideoPlayerWindow] Enviando orden de reproducción para URL:" << url;

    const char* cmd[] = { "loadfile", url.toUtf8().constData(), "replace", nullptr };
    // int status = mpv_command_async(mpv, 0, cmd);
    int status = mpv_command(mpv, cmd);

    if (status < 0) {
        qCritical() << "[VideoPlayerWindow] Error enviando el comando a mpv:" << status;
    }
}

void VideoPlayerWindow::onMpvEvents() {
    while (mpv) {
        mpv_event* event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) break;

        switch (event->event_id) {
        case MPV_EVENT_LOG_MESSAGE: {
            auto msg = static_cast<mpv_event_log_message*>(event->data);
            QString text = QString::fromUtf8(msg->text).trimmed();
            if (!text.isEmpty()) {
                qDebug() << "[MPV]" << msg->prefix << ":" << text;
            }
            break;
        }
        case MPV_EVENT_END_FILE: {
            auto eef = static_cast<mpv_event_end_file*>(event->data);
            qCritical() << "[MPV] Finalizó reproducción o falló. Razón code:" << eef->reason << "| Error:" << eef->error;
            break;
        }
        default:
            break;
        }
    }
}

void VideoPlayerWindow::stopMedia() {
    if (!mpv) return;

    qDebug() << "[VideoPlayerWindow] Deteniendo reproducción...";
    const char* cmd[] = { "stop", nullptr };
    mpv_command_async(mpv, 0, cmd);
}