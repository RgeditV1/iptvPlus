#ifndef VIDEOPLAYERWINDOW_HPP
#define VIDEOPLAYERWINDOW_HPP

#include <QWidget>
#include <mpv/client.h>

class VideoPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit VideoPlayerWindow(QWidget *parent = nullptr);
    ~VideoPlayerWindow();

    void playMedia(const QString &url);
    void stopMedia();

private:
    mpv_handle *mpv;

    void initMpv();
};

#endif // VIDEOPLAYERWINDOW_HPP