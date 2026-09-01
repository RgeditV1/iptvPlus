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
#include "channellistmodel.hpp"

class VideoPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit VideoPlayerWindow(QWidget* parent = nullptr);
    ~VideoPlayerWindow();

    void playMedia(const QString& url);
    void stopMedia();

    void setModel(ChannelListModel* model);

    void playChannelAt(int row);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onMpvEvents();

    void togglePlayPause();
    void setVolume(int value);
    void togleMute();
    void playPreviousChannel();
    void playNextChannel();
    void toggleFullScreen();

signals:
	void channelChanged(int row);
    void fullScreenToggled(bool isFullScreen);

private:
    mpv_handle* mpv;
    QTimer* mpvTimer;

    QWidget* videoContainer;
    QWidget* controlsContainer;

    QPushButton* btnPlayPause;
    QPushButton* btnStop;

    QPushButton* btnPrevious;
    QPushButton* btnNext;
    QPushButton* btnFullScreen;

    // Contenedor y controles de volumen alineados
    QWidget* volumeContainer;
    QToolButton* btnVolume;
    QSlider* sliderVolume;

    bool isPaused;
    bool isMuted;

    int volume;
    int previousVolume;

    ChannelListModel* channelModel; // <--- Puntero al modelo[cite: 3]
    int currentChannelRow;

    // Ocultamiento automático de la barra de controles
    QTimer* controlsTimer;

    void initMpv();
    void setupUi();

    void updateControls(bool show = true);
    void updateVolumeIcon();
};

#endif // VIDEOPLAYERWINDOW_HPP