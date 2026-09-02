#pragma once

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QSlider>
#include <QResizeEvent>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>

#include <mpv/client.h>
#include "channellistmodel.hpp"

class LoadingSpinner;

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
    void resizeEvent(QResizeEvent* event) override;

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

    QWidget* loadingOverlay;
    LoadingSpinner* loadingSpinner;
    QTimer* spinnerTimer;
    

    QPushButton* btnPrevious;
    QPushButton* btnNext;
    QPushButton* btnFullScreen;

    // Contenedor y controles de volumen alineados
    QWidget* volumeContainer;
    QToolButton* btnVolume;
    QSlider* sliderVolume;

    bool isPaused;
    bool isMuted;
    bool buffering = false;

    int volume;
    int previousVolume;
    int spinnerAngle;

    double cacheDuration = 0.0;


    ChannelListModel* channelModel; // <--- Puntero al modelo[cite: 3]
    int currentChannelRow;

    // Ocultamiento automático de la barra de controles
    QTimer* controlsTimer;

    void initMpv();
    void setupUi();

    void setupLoadingSpinner();
    void setLoadingSpinnerVisible(bool visible);
    void updateSpinner();

    void updateBufferingState();

    void updateControls(bool show = true);
    void updateVolumeIcon();
};

class LoadingSpinner : public QWidget
{
public:
    explicit LoadingSpinner(QWidget* parent = nullptr);

    void setAngle(int angle);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int angle = 0;
};