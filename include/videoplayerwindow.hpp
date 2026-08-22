#ifndef VIDEOPLAYERWINDOW_HPP
#define VIDEOPLAYERWINDOW_HPP

#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <QSlider>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>

#include <mpv/client.h>

class VideoPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit VideoPlayerWindow(QWidget* parent = nullptr);
    ~VideoPlayerWindow();

    void playMedia(const QString& url);
    void stopMedia();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onMpvEvents();

    void togglePlayPause();
    void setVolume(int value);
    void togleMute();

private:
    mpv_handle* mpv;
    QTimer* mpvTimer;

    QWidget* videoContainer;
    QWidget* controlsContainer;

    QPushButton* btnPlayPause;
    QPushButton* btnStop;

    // Contenedor y controles de volumen alineados
    QWidget* volumeContainer;
    QToolButton* btnVolume;
    QSlider* sliderVolume;

    bool isPaused;
    bool isMuted;

    int volume;
    int previousVolume;

    // Ocultamiento automático de la barra de controles
    QTimer* controlsTimer;

    void initMpv();
    void setupUi();

    void updateControls(bool show = true);
    void updateVolumeIcon();
};

#endif // VIDEOPLAYERWINDOW_HPP