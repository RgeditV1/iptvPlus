#include "videoplayerwindow.hpp"

#include "mpvplayer.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QIcon>
#include <QVBoxLayout>

VideoPlayerWindow::VideoPlayerWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("IPTV++");

    resize(1280, 720);

    setMouseTracking(true);

    setupUi();

    m_player = new MpvPlayer();

    m_controlsHideTimer = new QTimer(this);

    m_controlsHideTimer->setSingleShot(true);

    connect(
        m_controlsHideTimer,
        &QTimer::timeout,
        this,
        [this]()
        {
            if (m_isFullscreen) {
                m_controlsWidget->hide();
            }
        }
    );

    setupConnections();

    initializePlayer();
}

VideoPlayerWindow::~VideoPlayerWindow()
{
    delete m_player;
    m_player = nullptr;
}

bool VideoPlayerWindow::initializePlayer()
{
    if (!m_player)
        return false;

    if (!m_videoWidget)
        return false;

    const WId wid = m_videoWidget->winId();

    const bool initialized =
        m_player->initialize(wid);

    if (initialized) {

        const double currentVol =
            m_player->volume();

        m_volumeSlider->setValue(
            static_cast<int>(currentVol)
        );

        updateVolumeIcon(currentVol);
    }

    return initialized;
}

void VideoPlayerWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isFullscreen) {
        resetControlsHideTimer();
    }

    QWidget::mouseMoveEvent(event);
}

void VideoPlayerWindow::onBufferingChanged(bool buffering)
{
    if (buffering) {
        m_loadingSpinner->show();
    } else {
        m_loadingSpinner->hide();
    }
}

void VideoPlayerWindow::handlePlaybackError(const QString& error)
{
    m_loadingSpinner->hide();

    m_isPlaying = false;

    m_playButton->setIcon(
        QIcon(":/resources/icons/play.svg")
    );

    if (m_errorLabel) {

        m_errorLabel->setText(
            QString("Error de conexión/reproducción: %1")
                .arg(error)
        );

        m_errorLabel->show();
    }
}

void VideoPlayerWindow::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);

    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // ----------------------------------------
    // Área de vídeo
    // ----------------------------------------

    m_videoWidget = new QWidget(this);

    m_videoWidget->setMinimumSize(320, 180);

    m_videoWidget->setAttribute(
        Qt::WA_NativeWindow
    );

    m_videoWidget->setStyleSheet(
        "background-color: black;"
    );

    m_videoWidget->setMouseTracking(true);

    m_videoWidget->installEventFilter(this);

    // Layout superpuesto sobre el video para el spinner y errores
    QVBoxLayout* videoOverlayLayout = new QVBoxLayout(
        m_videoWidget
    );

    videoOverlayLayout->setAlignment(
        Qt::AlignCenter
    );

    m_loadingSpinner = new QProgressBar(
        m_videoWidget
    );

    m_loadingSpinner->setRange(0, 0); // Modo indeterminado (animación continua)

    m_loadingSpinner->setTextVisible(false);

    m_loadingSpinner->setFixedSize(60, 60);

    // Estilo circular/estilizado para el spinner
    m_loadingSpinner->setStyleSheet(
        "QProgressBar {"
        "   border: 4px solid #444444;"
        "   border-top: 4px solid #00adb5;"
        "   border-radius: 30px;"
        "   background-color: transparent;"
        "}"
    );

    m_loadingSpinner->hide();

    m_errorLabel = new QLabel(
        m_videoWidget
    );

    m_errorLabel->setStyleSheet(
        "QLabel {"
        "   color: #ff5555;"
        "   background-color: rgba(0, 0, 0, 180);"
        "   padding: 12px 20px;"
        "   border-radius: 8px;"
        "   font-weight: bold;"
        "   font-size: 14px;"
        "}"
    );

    m_errorLabel->hide();

    videoOverlayLayout->addWidget(m_loadingSpinner);
    videoOverlayLayout->addWidget(m_errorLabel);

    m_mainLayout->addWidget(
        m_videoWidget,
        1
    );

    // ----------------------------------------
    // Timeline
    // ----------------------------------------

    m_timeline = new QSlider(
        Qt::Horizontal,
        this
    );

    m_timeline->setRange(0, 1000);
    m_timeline->setValue(0);

    m_mainLayout->addWidget(
        m_timeline
    );

    // ----------------------------------------
    // Controles
    // ----------------------------------------

    m_controlsWidget = new QWidget(this);

    m_controlsLayout = new QHBoxLayout(
        m_controlsWidget
    );

    m_controlsLayout->setContentsMargins(
        8,
        4,
        8,
        4
    );

    m_currentTimeLabel = new QLabel(
        "00:00",
        this
    );

    m_durationLabel = new QLabel(
        "00:00",
        this
    );

    m_playButton = new QPushButton(this);
    m_playButton->setIcon(
        QIcon(":/resources/icons/play.svg")
    );

    m_stopButton = new QPushButton(this);
    m_stopButton->setIcon(
        QIcon(":/resources/icons/stop-circle.svg")
    );

    m_volumeButton = new QPushButton(this);

    m_volumeButton->setIcon(
        QIcon(":/resources/icons/volume-2.svg")
    );

    m_volumeSlider = new QSlider(
        Qt::Horizontal,
        this
    );

    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(100);

    m_fullscreenButton = new QPushButton(this);
    m_fullscreenButton->setIcon(
        QIcon(":/resources/icons/maximize.svg")
    );

    m_controlsLayout->addWidget(
        m_currentTimeLabel
    );

    m_controlsLayout->addWidget(
        m_playButton
    );

    m_controlsLayout->addWidget(
        m_stopButton
    );

    m_controlsLayout->addWidget(
        m_volumeButton
    );

    m_controlsLayout->addWidget(
        m_volumeSlider
    );

    m_controlsLayout->addWidget(
        m_durationLabel
    );

    m_controlsLayout->addStretch();

    m_controlsLayout->addWidget(
        m_fullscreenButton
    );

    m_mainLayout->addWidget(
        m_controlsWidget
    );
}

void VideoPlayerWindow::setupConnections()
{
    connect(
        m_player,
        &MpvPlayer::bufferingChanged,
        this,
        &VideoPlayerWindow::onBufferingChanged
    );

    connect(
        m_player,
        &MpvPlayer::playbackError,
        this,
        &VideoPlayerWindow::handlePlaybackError
    );

    connect(
        m_player,
        &MpvPlayer::positionChanged,
        this,
        &VideoPlayerWindow::updatePosition
    );

    connect(
        m_player,
        &MpvPlayer::durationChanged,
        this,
        &VideoPlayerWindow::updateDuration
    );

    connect(
        m_playButton,
        &QPushButton::clicked,
        this,
        &VideoPlayerWindow::togglePlayPause
    );

    connect(
        m_stopButton,
        &QPushButton::clicked,
        this,
        &VideoPlayerWindow::stop
    );

    connect(
        m_volumeButton,
        &QPushButton::clicked,
        this,
        &VideoPlayerWindow::toggleMute
    );

    connect(
        m_fullscreenButton,
        &QPushButton::clicked,
        this,
        &VideoPlayerWindow::toggleFullscreen
    );

    connect(
        m_volumeSlider,
        &QSlider::valueChanged,
        this,
        [this](int value)
        {
            if (!m_player)
                return;

            m_player->setVolume(
                static_cast<double>(value)
            );

            updateVolumeIcon(value);
        }
    );

    connect(
        m_player,
        &MpvPlayer::muteChanged,
        this,
        [this](bool muted)
        {
            if (muted) {

                m_volumeButton->setIcon(
                    QIcon(":/resources/icons/volume-x.svg")
                );

            } else {

                const double currentVol =
                    m_player->volume();

                updateVolumeIcon(currentVol);
            }
        }
    );

    connect(
        m_timeline,
        &QSlider::sliderReleased,
        this,
        [this]()
        {
            if (!m_player)
                return;

            const double duration =
                m_player->duration();

            if (duration <= 0.0)
                return;

            const double position =
                duration * m_timeline->value() / 1000.0;

            m_player->seek(position);
        }
    );
}

bool VideoPlayerWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (m_isFullscreen && event->type() == QEvent::MouseMove) {
        resetControlsHideTimer();
    }
    return QWidget::eventFilter(watched, event);
}

void VideoPlayerWindow::resetControlsHideTimer()
{
    if (!m_isFullscreen)
        return;

    m_controlsWidget->show();

    m_controlsHideTimer->start(2500);
}

void VideoPlayerWindow::play(const QString& url)
{
    if (!m_player)
        return;

    if (url.isEmpty())
        return;

    if (m_errorLabel) {
        m_errorLabel->hide();
    }

    if (m_loadingSpinner) {
        m_loadingSpinner->show();
        m_loadingSpinner->raise(); // Asegura que se renderice sobre la superficie de mpv
    }

    m_player->play(url);

    m_isPlaying = true;

    m_playButton->setIcon(
        QIcon(":/resources/icons/pause.svg")
    );
}

void VideoPlayerWindow::pause()
{
    if (!m_player)
        return;

    m_player->pause();

    m_isPlaying = false;

    m_playButton->setIcon(
        QIcon(":/resources/icons/play.svg")
    );
}

void VideoPlayerWindow::stop()
{
    if (!m_player)
        return;

    m_player->stop();

    m_isPlaying = false;

    m_playButton->setIcon(
        QIcon(":/resources/icons/play.svg")
    );

    m_timeline->setValue(0);

    if (m_errorLabel) {
        m_errorLabel->hide();
    }
}

void VideoPlayerWindow::togglePlayPause()
{
    if (!m_player)
        return;

    if (m_isPlaying) {
        pause();
    } else {
        m_player->resume();

        m_isPlaying = true;

        m_playButton->setIcon(
            QIcon(":/resources/icons/pause.svg")
        );
    }
}

void VideoPlayerWindow::toggleMute()
{
    if (!m_player)
        return;

    const bool muted =
        m_player->isMuted();

    m_player->setMuted(!muted);
}

void VideoPlayerWindow::toggleFullscreen()
{
    QWidget* topLevel = window(); // Obtener la ventana principal

    if (m_isFullscreen) {
        m_controlsHideTimer->stop();
        m_controlsWidget->show();

        if (topLevel) {
            topLevel->showNormal();
        } else {
            showNormal();
        }

        m_isFullscreen = false;
        m_fullscreenButton->setIcon(QIcon(":/resources/icons/maximize.svg"));

    } else {
        if (topLevel) {
            topLevel->showFullScreen();
        } else {
            showFullScreen();
        }

        m_isFullscreen = true;
        m_fullscreenButton->setIcon(QIcon(":/resources/icons/minimize.svg"));

        resetControlsHideTimer();
    }
}

void VideoPlayerWindow::updateVolumeIcon(double volume)
{
    if (m_player && m_player->isMuted()) {

        m_volumeButton->setIcon(
            QIcon(":/resources/icons/volume-x.svg")
        );

        return;
    }

    if (volume <= 0.0) {

        m_volumeButton->setIcon(
            QIcon(":/resources/icons/volume-x.svg")
        );

    } else if (volume < 10.0) {

        m_volumeButton->setIcon(
            QIcon(":/resources/icons/volume.svg")
        );

    } else if (volume < 60.0) {

        m_volumeButton->setIcon(
            QIcon(":/resources/icons/volume-1.svg")
        );

    } else {

        m_volumeButton->setIcon(
            QIcon(":/resources/icons/volume-2.svg")
        );

    }
}

void VideoPlayerWindow::updatePosition(double position)
{
    if (!m_player)
        return;

    const double duration = m_player->duration();

    if (duration <= 0.0) {
        m_currentTimeLabel->setText("EN VIVO");
        m_timeline->setEnabled(false);
        return;
    }

    m_timeline->setEnabled(true);

    if (!m_timeline->isSliderDown()) {
        const int sliderValue = static_cast<int>(
            (position / duration) * 1000.0
        );

        m_timeline->setValue(sliderValue);
    }

    const int totalSeconds =
        static_cast<int>(position);

    const int minutes =
        totalSeconds / 60;

    const int seconds =
        totalSeconds % 60;

    m_currentTimeLabel->setText(
        QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
    );
}

void VideoPlayerWindow::updateDuration(double duration)
{
    if (duration <= 0.0) {

        m_durationLabel->setText("LIVE");
        m_timeline->setEnabled(false);
        m_currentTimeLabel->setText("EN VIVO");

        return;
    }

    m_timeline->setEnabled(true);

    const int totalSeconds =
        static_cast<int>(duration);

    const int minutes =
        totalSeconds / 60;

    const int seconds =
        totalSeconds % 60;

    m_durationLabel->setText(
        QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
    );
}