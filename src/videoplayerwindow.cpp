#include "videoplayerwindow.hpp"
#include <QDebug>

VideoPlayerWindow::VideoPlayerWindow(QWidget *parent)
    : QWidget(parent), mpv(nullptr)
{
    setWindowTitle("IPTV Plus - Reproductor");
    resize(800, 450);
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
        qCritical() << "No se pudo crear la instancia de mpv.";
        return;
    }

    // Usar la ventana de este Widget para renderizar el video
    int64_t wid = static_cast<int64_t>(winId());
    mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);

    if (mpv_initialize(mpv) < 0) {
        qCritical() << "No se pudo inicializar mpv.";
        return;
    }
}

void VideoPlayerWindow::playMedia(const QString &url) {
    if (!mpv) return;

    const char *cmd[] = {"loadfile", url.toUtf8().constData(), nullptr};
    mpv_command_async(mpv, 0, cmd);
}

void VideoPlayerWindow::stopMedia() {
    if (!mpv) return;

    const char *cmd[] = {"stop", nullptr};
    mpv_command_async(mpv, 0, cmd);
}