#pragma once

#include <QMouseEvent>
#include <QProgressBar>
#include <QString>
#include <QTimer>
#include <QWidget>

class QPushButton;
class QSlider;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

class MpvPlayer;

class VideoPlayerWindow : public QWidget {
  Q_OBJECT

public:
  explicit VideoPlayerWindow(QWidget *parent = nullptr);
  ~VideoPlayerWindow() override;

  bool initializePlayer();

  void play(const QString &url);
  void pause();
  void stop();

protected:
  void mouseMoveEvent(QMouseEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void onBufferingChanged(bool buffering);
  void handlePlaybackError(const QString &error);

private:
  void setupUi();
  void setupConnections();

  void togglePlayPause();
  void toggleMute();
  void toggleFullscreen();

  void updatePosition(double position);
  void updateDuration(double duration);
  void updateVolumeIcon(double volume);

  void resetControlsHideTimer();

private:
  MpvPlayer *m_player = nullptr;

  // Área donde libmpv renderizará el vídeo
  QWidget *m_videoWidget = nullptr;
  QWidget *m_controlsWidget = nullptr;

  // Controles
  QPushButton *m_playButton = nullptr;
  QPushButton *m_stopButton = nullptr;
  QPushButton *m_fullscreenButton = nullptr;
  QPushButton *m_volumeButton = nullptr;

  QTimer *m_controlsHideTimer = nullptr;

  QSlider *m_timeline = nullptr;
  QSlider *m_volumeSlider = nullptr;

  QLabel *m_currentTimeLabel = nullptr;
  QLabel *m_durationLabel = nullptr;
  QLabel *m_errorLabel = nullptr;

  QVBoxLayout *m_mainLayout = nullptr;
  QHBoxLayout *m_controlsLayout = nullptr;

  QProgressBar *m_loadingSpinner = nullptr;

  bool m_isPlaying = false;
  bool m_isFullscreen = false;
};
