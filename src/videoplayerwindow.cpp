#include "videoplayerwindow.hpp"

#include <cstring>

#include <QApplication>
#include  <QIcon>
#include <QDebug>
#include <QPainter>
#include <QPen>

VideoPlayerWindow::VideoPlayerWindow(QWidget* parent)
    : QWidget(parent),
    mpv(nullptr),
    mpvTimer(nullptr),
    videoContainer(nullptr),
    controlsContainer(nullptr),
    btnPrevious(nullptr),
    btnNext(nullptr),
    btnPlayPause(nullptr),
    btnStop(nullptr),
    volumeContainer(nullptr),
    btnVolume(nullptr),
    sliderVolume(nullptr),
    isPaused(false),
    isMuted(false),
    volume(100),
    previousVolume(100),
    channelModel(nullptr),
	currentChannelRow(-1), // sin canal por defecto
    controlsTimer(nullptr)
{
    setMouseTracking(true);

    setupUi();
    initMpv();

    // Timer para ocultar la barra de controles principal
    controlsTimer = new QTimer(this);
    controlsTimer->setSingleShot(true);

    connect(
        controlsTimer,
        &QTimer::timeout,
        this,
        [this]() {
            updateControls(false);
        }
    );
}

VideoPlayerWindow::~VideoPlayerWindow()
{
    if (controlsTimer)
        controlsTimer->stop();

    if (mpvTimer)
        mpvTimer->stop();

    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = nullptr;
    }
}

void VideoPlayerWindow::setupUi()
{
    this->setStyleSheet("VideoPlayerWindow { background-color: black; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // =========================================================
    // CONTENEDOR DEL VIDEO
    // =========================================================

    videoContainer = new QWidget(this);

    videoContainer->setAttribute(Qt::WA_NativeWindow, true);
    videoContainer->setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    videoContainer->setMouseTracking(true);
    videoContainer->setStyleSheet("background-color: black;");

    // =========================================================
    // CARGA DE SPINNER
    // =========================================================

    setupLoadingSpinner();

    // =========================================================
    // BARRA DE CONTROLES
    // =========================================================

    controlsContainer = new QWidget(this);
    controlsContainer->setObjectName("controlsContainer");
    controlsContainer->setAttribute(Qt::WA_StyledBackground, true);
    controlsContainer->setMouseTracking(true);

    controlsContainer->setStyleSheet(
        "#controlsContainer {"
        "    background-color: rgba(45, 45, 50, 120);"
        "    border: none;"
        "}"

        "#controlsContainer QPushButton,"
        "#controlsContainer QToolButton {"
        "    color: white;"
        "    background-color: transparent;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "}"

        "#controlsContainer QPushButton:hover,"
        "#controlsContainer QToolButton:hover {"
        "    background-color: rgba(255, 255, 255, 40);"
        "}"

        "#volumeContainer {"
        "    background-color: transparent;"
        "    border: none;"
        "}"

        "#volumeContainer QToolButton {"
        "    background-color: transparent;"
        "    border: none;"
        "}"

        "#volumeContainer QSlider {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
    );

    btnBackToDetails = new QPushButton("← Atras", controlsContainer);
    btnBackToDetails->setCursor(Qt::PointingHandCursor);
    btnBackToDetails->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(43, 43, 54, 200);"
        "    border: 1px solid #3a3a4c;"
        "    border-radius: 4px;"
        "    padding: 4px 10px;"
        "    color: white;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: rgba(58, 58, 76, 255); }"
    );
    btnBackToDetails->hide();

    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsContainer);
    controlsLayout->setContentsMargins(8, 6, 8, 6);
    controlsLayout->setSpacing(5);

    btnPlayPause = new QPushButton(controlsContainer);
    btnPlayPause->setCursor(Qt::PointingHandCursor);
    btnPlayPause->setIcon(QIcon(":/resources/icons/play.svg"));

    btnStop = new QPushButton(controlsContainer);
    btnStop->setCursor(Qt::PointingHandCursor);
    btnStop->setIcon(QIcon(":/resources/icons/stop-circle.svg"));

    btnNext = new QPushButton(controlsContainer);
    btnNext->setCursor(Qt::PointingHandCursor);
    btnNext->setIcon(QIcon(":/resources/icons/skip-forward.svg"));

    btnPrevious = new QPushButton(controlsContainer);
    btnPrevious->setCursor(Qt::PointingHandCursor);
    btnPrevious->setIcon(QIcon(":/resources/icons/skip-back.svg"));

    btnFullScreen = new QPushButton(controlsContainer);
    btnFullScreen->setCursor(Qt::PointingHandCursor);
    btnFullScreen->setIcon(QIcon(":/resources/icons/maximize.svg"));

    // =========================================================
    // CONTENEDOR DE VOLUMEN (ICONO Y SLIDER EN LÍNEA)
    // =========================================================

    volumeContainer = new QWidget(controlsContainer);
    volumeContainer->setMouseTracking(true);
    volumeContainer->setAttribute(Qt::WA_TranslucentBackground);
    volumeContainer->setStyleSheet("background: transparent;");
    // Ancho fijo: 32px (botón) + 5px (espacio) + 100px (slider) = 137px
    volumeContainer->setFixedSize(137, 32);

    btnVolume = new QToolButton(volumeContainer);
    btnVolume->setIconSize(QSize(25, 25));
    btnVolume->setCursor(Qt::PointingHandCursor);
    // El botón se queda FIJO en la esquina izquierda (0,0) y no se moverá jamás
    btnVolume->setGeometry(0, 0, 32, 32);

    // Slider de volumen horizontal
    sliderVolume = new QSlider(Qt::Horizontal, volumeContainer);
    sliderVolume->setRange(0, 100);
    sliderVolume->setValue(100);

    // Posicionamos el slider exactamente a 5px de separación del botón (32px + 5px = 37px)
    sliderVolume->setGeometry(37, 6, 100, 20);
    sliderVolume->setCursor(Qt::PointingHandCursor);


    sliderVolume->hide();


    // =========================================================
    // AGREGAR AL LAYOUT DE CONTROLES
    // =========================================================
    controlsLayout->addWidget(btnBackToDetails);
    controlsLayout->addStretch(1);

    controlsLayout->addWidget(btnPrevious);
    controlsLayout->addWidget(btnPlayPause);
    controlsLayout->addWidget(btnNext);
    controlsLayout->addWidget(btnStop);

    controlsLayout->addStretch(1);

    controlsLayout->addWidget(volumeContainer);
    controlsLayout->addWidget(btnFullScreen);

    // =========================================================
    // LAYOUT PRINCIPAL
    // =========================================================

    mainLayout->addWidget(videoContainer, 1);
    mainLayout->addWidget(controlsContainer, 0);

    // =========================================================
    // REGISTRO DE EVENT FILTERS
    // =========================================================

    videoContainer->installEventFilter(this);
    controlsContainer->installEventFilter(this);
    btnPlayPause->installEventFilter(this);
    btnStop->installEventFilter(this);
    btnPrevious->installEventFilter(this);
    btnNext->installEventFilter(this);
    volumeContainer->installEventFilter(this);
    btnVolume->installEventFilter(this);
    sliderVolume->installEventFilter(this);
    btnFullScreen->installEventFilter(this);

    // =========================================================
    // CONEXIONES
    // =========================================================

    connect(btnPlayPause, &QPushButton::clicked, this, &VideoPlayerWindow::togglePlayPause);
    connect(btnFullScreen, &QPushButton::clicked, this, &VideoPlayerWindow::toggleFullScreen);
    connect(btnPrevious, &QPushButton::clicked, this, &VideoPlayerWindow::playPreviousChannel);
    connect(btnNext, &QPushButton::clicked, this, &VideoPlayerWindow::playNextChannel);
    connect(sliderVolume, &QSlider::valueChanged, this, &VideoPlayerWindow::setVolume);
    connect(btnVolume, &QToolButton::clicked, this, &VideoPlayerWindow::togleMute);
    connect(btnStop, &QPushButton::clicked, this, &VideoPlayerWindow::stopMedia);
    connect(btnBackToDetails, &QPushButton::clicked, this, &VideoPlayerWindow::backToDetailsRequested);

    updateVolumeIcon();
}

void VideoPlayerWindow::setModel(ChannelListModel* model)
{
    channelModel = model;
    currentChannelRow = -1;
}

void VideoPlayerWindow::playChannelAt(int row)
{
    if (!channelModel) {
        qWarning() << "[VideoPlayerWindow] No hay un ChannelListModel asignado.";
        return;
    }

    const M3UItem* item = channelModel->channelAt(row); // Obtiene el ítem de la fila[cite: 3, 5]
    if (!item) {
        qWarning() << "[VideoPlayerWindow] Fila inválida en el modelo de canales:" << row;
        return;
    }

    currentChannelRow = row;
    qDebug() << "[VideoPlayerWindow] Reproduciendo canal:" << item->title << "URL:" << item->url;
    playMedia(item->url);
    emit channelChanged(row);
}

void VideoPlayerWindow::playNextChannel()
{
    if (!channelModel || channelModel->rowCount() == 0) { // Consulta el total del modelo[cite: 3, 5]
        qWarning() << "[VideoPlayerWindow] El modelo de canales está vacío o no asignado.";
        return;
    }

    int totalRows = channelModel->rowCount(); //[cite: 3, 5]
    // Avanzar a la siguiente fila (con loop circular)
    int nextRow = (currentChannelRow + 1) % totalRows;

    playChannelAt(nextRow);
}

void VideoPlayerWindow::playPreviousChannel()
{
    if (!channelModel || channelModel->rowCount() == 0) { //[cite: 3, 5]
        qWarning() << "[VideoPlayerWindow] El modelo de canales está vacío o no asignado.";
        return;
    }

    int totalRows = channelModel->rowCount(); //[cite: 3, 5]
    // Retroceder a la fila anterior (con loop circular)
    int prevRow = (currentChannelRow - 1 + totalRows) % totalRows;

    playChannelAt(prevRow);
}

void VideoPlayerWindow::initMpv() {
    mpv = mpv_create();
    if (!mpv) {
        qCritical() << "[VideoPlayerWindow] No se pudo crear la instancia de mpv.";
        return;
    }

    mpv_set_option_string(mpv, "terminal", "no");
    mpv_set_option_string(mpv, "msg-level", "all=warn");
    mpv_set_option_string(mpv, "vo", "gpu");

#if defined(Q_OS_WIN)
    mpv_set_option_string(mpv, "gpu-context", "d3d11");
#else
    mpv_set_option_string(mpv, "gpu-context", "auto");
#endif

    mpv_set_option_string(mpv, "user-agent", "Mozilla/5.0");
    mpv_set_option_string(mpv, "tls-verify", "no");

    int64_t wid = static_cast<int64_t>(videoContainer->winId());
    mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);

    if (mpv_initialize(mpv) < 0) {
        qCritical() << "[VideoPlayerWindow] No se pudo inicializar mpv.";
        return;
    }

    mpv_observe_property(
        mpv,
        0,
        "demuxer-cache-duration",
        MPV_FORMAT_DOUBLE
    );

    mpv_observe_property(
        mpv,
        0,
        "paused",
        MPV_FORMAT_FLAG
    );

    // Detecta cuando MPV está inactivo o cargando un recurso nuevo/red
    mpv_observe_property(
        mpv,
        0,
        "core-idle",
        MPV_FORMAT_FLAG
    );
    mpv_request_log_messages(mpv, "info");

    mpvTimer = new QTimer(this);
    connect(mpvTimer, &QTimer::timeout, this, &VideoPlayerWindow::onMpvEvents);
    mpvTimer->start(10);

    qDebug() << "[VideoPlayerWindow] Instancia MPV inicializada correctamente.";
}

void VideoPlayerWindow::togglePlayPause()
{
    if (!mpv)
        return;

    isPaused = !isPaused;
    int pauseValue = isPaused ? 1 : 0;

    int status = mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pauseValue);

    if (status < 0) {
        qWarning() << "[VideoPlayerWindow] No se pudo cambiar pause:" << mpv_error_string(status);
        return;
    }

    btnPlayPause->setIcon(isPaused ? QIcon(":/resources/icons/play.svg") : QIcon(":/resources/icons/pause.svg"));
    updateControls(true);
}

void VideoPlayerWindow::toggleFullScreen()
{
    QWidget* targetWindow = this->topLevelWidget();

    bool goesFullScreen = !targetWindow->isFullScreen();

    if (targetWindow->isFullScreen()) {
        targetWindow->showNormal();
        if (btnFullScreen) {
            btnFullScreen->setIcon(QIcon(":/resources/icons/maximize.svg"));
        }
    } else {
        targetWindow->showFullScreen();
        if (btnFullScreen) {
            btnFullScreen->setIcon(QIcon(":/resources/icons/minimize.svg"));
        }
    }

    emit fullScreenToggled(goesFullScreen);
}

void VideoPlayerWindow::setVolume(int value)
{
    if (!mpv)
        return;

    volume = value;

    if (value > 0) {
        previousVolume = value;
        isMuted = false;
    }
    else {
        isMuted = true;
    }

    double mpvVolume = static_cast<double>(value);

    const int status = mpv_set_property(
        mpv,
        "volume",
        MPV_FORMAT_DOUBLE,
        &mpvVolume
    );

    if (status < 0) {
        qWarning()
            << "[VideoPlayerWindow] No se pudo cambiar volumen:"
            << mpv_error_string(status);
        return;
    }

    updateVolumeIcon();
}

void VideoPlayerWindow::togleMute()
{
    isMuted = !isMuted;

    if (isMuted) {
        previousVolume = volume;
        setVolume(0);
    }
    else {
        setVolume(previousVolume > 0 ? previousVolume : 100);
    }

    sliderVolume->setValue(volume);
}

void VideoPlayerWindow::updateVolumeIcon()
{
    QString iconPath;

    if (volume <= 0 || isMuted) {
        iconPath = ":/resources/icons/volume-x.svg";
    }
    else if (volume <= 10) {
        iconPath = ":/resources/icons/volume.svg";
    }
    else if (volume <= 60) {
        iconPath = ":/resources/icons/volume-1.svg";
    }
	else{
		iconPath = ":/resources/icons/volume-2.svg";
	}

    btnVolume->setIcon(QIcon(iconPath));
}

void VideoPlayerWindow::updateControls(bool show)
{
    if (!controlsContainer)
        return;

    if (show) {
        controlsContainer->show();

        if (controlsTimer && !controlsContainer->underMouse())
            controlsTimer->start(3000);

        return;
    }

    if (controlsTimer)
        controlsTimer->stop();

    controlsContainer->hide();
}

bool VideoPlayerWindow::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {

    case QEvent::Enter:
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
        updateControls(true);
        break;

    default:
        break;
    }

    if (watched == controlsContainer) {
        if (event->type() == QEvent::Enter) {
            // Cancelar la ocultación automática mientras el usuario navega sobre los botones
            if (controlsTimer)
                controlsTimer->stop();
            updateControls(true);
            return QWidget::eventFilter(watched, event);
        }
        else if (event->type() == QEvent::Leave) {
            // Reiniciar la cuenta regresiva al salir del área de controles
            if (controlsTimer)
                controlsTimer->start(3000);
            return QWidget::eventFilter(watched, event);
        }
    }

    if (watched == btnVolume && event->type() == QEvent::Enter) {
        sliderVolume->show();
    }

    if (watched == volumeContainer && event->type() == QEvent::Leave) {
        const QPoint pos =
            volumeContainer->mapFromGlobal(QCursor::pos());

        if (!volumeContainer->rect().contains(pos)) {
            sliderVolume->hide();
        }
    }

    return QWidget::eventFilter(watched, event);
}

void VideoPlayerWindow::setNavigationButtonsVisible(bool visible)
{
    if (btnPrevious) {
        btnPrevious->setVisible(visible);
    }
    if (btnNext) {
        btnNext->setVisible(visible);
    }
}

void VideoPlayerWindow::setBackToDetailsVisible(bool visible) {
    if (btnBackToDetails) {
        btnBackToDetails->setVisible(visible);
    }
}

void VideoPlayerWindow::playMedia(const QString& url)
{
    if (!mpv) {
        qWarning() << "[VideoPlayerWindow] No se puede reproducir: la instancia MPV no existe.";
        return;
    }

    qDebug() << "[VideoPlayerWindow] Enviando orden de reproducción para URL:" << url;

    isPaused = false;
    int pauseValue = 0;
    mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pauseValue);


    buffering = true;
    cacheDuration = 0.0;
    setLoadingSpinnerVisible(true);
    QByteArray urlData = url.toUtf8();

    const char* cmd[] = {
        "loadfile",
        urlData.constData(),
        "replace",
        nullptr
    };

    int status = mpv_command_async(mpv, 0, cmd);

    if (status < 0) {
        qCritical() << "[VideoPlayerWindow] Error enviando el comando a mpv:" << status << mpv_error_string(status);
    }

    isPaused = false;
	btnPlayPause->setIcon(QIcon(":/resources/icons/pause.svg"));
}

void VideoPlayerWindow::onMpvEvents()
{
    if (!mpv)
        return;

    while (true) {
        mpv_event* event = mpv_wait_event(mpv, 0);

        if (!event || event->event_id == MPV_EVENT_NONE)
            break;

        switch (event->event_id) {

        case MPV_EVENT_PROPERTY_CHANGE:
        {
            auto* prop = static_cast<mpv_event_property*>(event->data);

            if (!prop || !prop->name)
                break;

            if (strcmp(prop->name, "demuxer-cache-duration") == 0) {

                if (prop->format == MPV_FORMAT_DOUBLE && prop->data) {
                    cacheDuration = *static_cast<double*>(prop->data);

                    //qDebug() << "[MPV] Cache:"
                     //       << cacheDuration
                       //     << "segundos";

                    updateBufferingState();
                }
            }
            else if (strcmp(prop->name, "paused") == 0) {

                if (prop->format == MPV_FORMAT_FLAG && prop->data) {
                    isPaused = *static_cast<int*>(prop->data);

                    updateBufferingState();
                }
            }

            else if (strcmp(prop->name, "core-idle") == 0) {
                if (prop->format == MPV_FORMAT_FLAG && prop->data) {
                    bool isIdle = *static_cast<int*>(prop->data);
                    
                    // Si mpv entra en idle durante la carga, forzamos la visibilidad del spinner
                    if (isIdle && !isPaused) {
                        buffering = true;
                        setLoadingSpinnerVisible(true);
                    }
                }
            }

            break;
        }
        
        // Si el archivo empieza a reproducirse correctamente
        case MPV_EVENT_PLAYBACK_RESTART:
        {
            // El video ha comenzado a emitir frames, si ya tenemos buffer razonable oculta el spinner
            if (cacheDuration >= 1.0) {
                buffering = false;
                setLoadingSpinnerVisible(false);
            }
            break;
        }

        case MPV_EVENT_LOG_MESSAGE:
        {
            auto* msg = static_cast<mpv_event_log_message*>(event->data);

            if (msg && msg->text) {
                QString text = QString::fromUtf8(msg->text).trimmed();

                if (!text.isEmpty()) {
                    qDebug() << "[MPV]" << msg->prefix << ":" << text;
                }
            }
            break;
        }

        case MPV_EVENT_END_FILE:
        {
            auto* eef = static_cast<mpv_event_end_file*>(event->data);

            if (eef) {
                qDebug() << "[MPV] Fin de reproducción. Razón:" << eef->reason << "| Error:" << eef->error;
            }
            break;
        }

        default:
            break;
        }
    }
}

void VideoPlayerWindow::stopMedia()
{
    if (!mpv)
        return;

    qDebug() << "[VideoPlayerWindow] Deteniendo reproducción...";

    const char* cmd[] = {
        "stop",
        nullptr
    };

    int status = mpv_command(mpv, cmd);

    if (status < 0) {
        qWarning() << "[VideoPlayerWindow] Error deteniendo MPV:" << mpv_error_string(status);
    }

    isPaused = false;
	btnPlayPause->setIcon(QIcon(":/resources/icons/play.svg"));
}

void VideoPlayerWindow::updateBufferingState()
{
    if (!mpv)
        return;

    if (isPaused && cacheDuration > 0.0) {
        if (buffering) {
            buffering = false;
            setLoadingSpinnerVisible(false);
        }
        return;
    }


    if (buffering && cacheDuration >= 2.0) {
        buffering = false;
        setLoadingSpinnerVisible(false);
    }

    else if (!buffering && cacheDuration < 0.5 && !isPaused) {
        buffering = true;
        setLoadingSpinnerVisible(true);
    }
}

void VideoPlayerWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (!videoContainer || !loadingOverlay || !loadingSpinner)
        return;

    loadingOverlay->setGeometry(videoContainer->rect());

    loadingSpinner->move(
        (loadingOverlay->width() - loadingSpinner->width()) / 2,
        (loadingOverlay->height() - loadingSpinner->height()) / 2
    );
}

void VideoPlayerWindow::setLoadingSpinnerVisible(bool visible)
{
    if (visible) {
        loadingOverlay->setGeometry(videoContainer->rect());

        loadingSpinner->move(
            (loadingOverlay->width() - loadingSpinner->width()) / 2,
            (loadingOverlay->height() - loadingSpinner->height()) / 2
        );

        loadingOverlay->show();
        loadingOverlay->raise();

        spinnerTimer->start(50);
    }
    else {
        spinnerTimer->stop();
        loadingOverlay->hide();
    }
}

void VideoPlayerWindow::setupLoadingSpinner()
{
    loadingOverlay = new QWidget(videoContainer);

    loadingOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    loadingOverlay->setStyleSheet(
        "#loadingOverlay {"
        "    background: transparent;"
        "}"
    );

    loadingSpinner = new LoadingSpinner(loadingOverlay);

    spinnerAngle = 0;

    spinnerTimer = new QTimer(this);

    connect(spinnerTimer, &QTimer::timeout,
            this, &VideoPlayerWindow::updateSpinner);

    loadingOverlay->hide();
}

LoadingSpinner::LoadingSpinner(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedSize(60, 60);
}

void VideoPlayerWindow::updateSpinner()
{
    spinnerAngle += 45;

    if (spinnerAngle >= 360)
        spinnerAngle = 0;

    loadingSpinner->setAngle(spinnerAngle);
}

void LoadingSpinner::setAngle(int value)
{
    angle = value;
    update();
}

void LoadingSpinner::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    const QPointF center = rect().center();

    painter.translate(center);
    painter.rotate(angle);

    const int radius = 20;
    const int lineWidth = 4;

    for (int i = 0; i < 8; ++i) {
        painter.save();

        painter.rotate(i * 45);

        int alpha = 40 + (i * 25);

        QPen pen;
        pen.setWidth(lineWidth);
        pen.setCapStyle(Qt::RoundCap);
        pen.setColor(QColor(255, 255, 255, alpha));

        painter.setPen(pen);

        painter.drawLine(
            0,
            -radius + 5,
            0,
            -radius + 12
        );

        painter.restore();
    }
}