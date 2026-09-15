#include "moviedetail.hpp"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QPixmap>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QScrollArea>
#include <QOverload>

MovieDetailWidget::MovieDetailWidget(QWidget* parent)
    : QWidget(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_torrentEngine = new TorrentEngine(this);

    setupUi();

    connect(m_torrentEngine, &TorrentEngine::readyToPlay, this, &MovieDetailWidget::onTorrentReadyToPlay);
    connect(m_torrentEngine, &TorrentEngine::errorOccurred, this, &MovieDetailWidget::onTorrentError);
    connect(m_torrentEngine, &TorrentEngine::progressUpdated, this,
            [this](float progress, int downloadRate, int numPeers) {
                if (!m_playButton->isEnabled() && m_playButton->text() != "Reproduciendo...") {
                    double rateKb = downloadRate / 1024.0;
                    QString rateStr = (rateKb >= 1024.0)
                        ? QString::number(rateKb / 1024.0, 'f', 1) + " MB/s"
                        : QString::number(rateKb, 'f', 0) + " KB/s";

                    m_playButton->setText(
                        QString("Cargando %1% (%2) [%3 peers]")
                            .arg(progress, 0, 'f', 1)
                            .arg(rateStr)
                            .arg(numPeers)
                    );
                }
            });

    connect(m_torrentEngine, &TorrentEngine::metadataLoaded, this,
            [this](const QString& fileName, qint64 fileSize) {
                qDebug() << "[MovieDetailWidget] Vídeo encontrado:" << fileName << "tamaño:" << fileSize;
                m_playButton->setText("Preparando piezas iniciales...");
            });
}

MovieDetailWidget::~MovieDetailWidget()
{
    if (m_torrentEngine) {
        m_torrentEngine->stop();
    }
    if (m_videoPlayer) {
        m_videoPlayer->stop();
    }
}

void MovieDetailWidget::setupUi()
{
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; }");

    QWidget* container = new QWidget(scrollArea);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // ------------------------------------------------
    // Top Bar (Botón volver)
    // ------------------------------------------------
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    m_backButton = new QPushButton("< Volver", container);
    m_backButton->setCursor(Qt::PointingHandCursor);
    m_backButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #313244;"
        "  color: #cdd6f4;"
        "  border: none;"
        "  padding: 8px 16px;"
        "  border-radius: 5px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45475a;"
        "}"
    );

    topBarLayout->addWidget(m_backButton);
    topBarLayout->addStretch();
    mainLayout->addLayout(topBarLayout);

    connect(m_backButton, &QPushButton::clicked, this, [this]() {
        if (m_torrentEngine) m_torrentEngine->stop();
        if (m_videoPlayer) m_videoPlayer->stop();
        emit backRequested();
    });

    // ------------------
    // Detalle Película
    // ------------------
    QHBoxLayout* infoLayout = new QHBoxLayout();
    infoLayout->setSpacing(15);

    // Póster con marco destacado
    m_posterLabel = new QLabel(container);
    m_posterLabel->setFixedSize(180, 270);
    m_posterLabel->setStyleSheet(
        "QLabel {"
        "  background-color: #11111b;"
        "  border: 2px solid #45475a;"
        "  border-radius: 8px;"
        "}"
    );
    m_posterLabel->setScaledContents(false);
    m_posterLabel->setAlignment(Qt::AlignCenter);

    infoLayout->addWidget(m_posterLabel, 0, Qt::AlignTop);

    QVBoxLayout* metaLayout = new QVBoxLayout();
    metaLayout->setContentsMargins(0, 0, 0, 0);
    metaLayout->setSpacing(8);
    metaLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // TÍTULO: Ajuste de política de tamaño para expansión vertical
    m_titleLabel = new QLabel(container);
    m_titleLabel->setStyleSheet("color: #ffffff; font-size: 22px; font-weight: bold;");
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    m_metaLabel = new QLabel(container);
    m_metaLabel->setStyleSheet("color: #89b4fa; font-size: 13px; font-weight: bold;");

    // Layout horizontal de Géneros pegado a la izquierda
    m_genresLayout = new QHBoxLayout();
    m_genresLayout->setContentsMargins(0, 0, 0, 0);
    m_genresLayout->setSpacing(6);
    m_genresLayout->setAlignment(Qt::AlignLeft);

    // DESCRIPCIÓN: Ajuste de política de tamaño para expansión vertical
    m_descriptionLabel = new QLabel(container);
    m_descriptionLabel->setStyleSheet("color: #a6adc8; font-size: 13px; line-height: 1.4;");
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    m_torrentSelector = new QComboBox(container);
    m_torrentSelector->setFixedWidth(320);
    m_torrentSelector->setStyleSheet(
        "QComboBox {"
        "  background-color: #1e1e2e;"
        "  color: #cdd6f4;"
        "  border: 1px solid #45475a;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "}"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView {"
        "  background-color: #181825;"
        "  color: #cdd6f4;"
        "  selection-background-color: #45475a;"
        "}"
    );

    m_playButton = new QPushButton("Reproducción Torrent", container);
    m_playButton->setCursor(Qt::PointingHandCursor);
    m_playButton->setFixedWidth(260);
    m_playButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #a6e3a1;"
        "  color: #11111b;"
        "  font-weight: bold;"
        "  font-size: 14px;"
        "  border: none;"
        "  padding: 8px;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover { background-color: #94e2d5; }"
        "QPushButton:disabled { background-color: #45475a; color: #bac2de; }"
    );

    connect(m_playButton, &QPushButton::clicked, this, &MovieDetailWidget::startSelectedTorrent);
    connect(m_torrentSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0) startSelectedTorrent();
    });

    metaLayout->addWidget(m_titleLabel);
    metaLayout->addWidget(m_metaLabel);
    metaLayout->addLayout(m_genresLayout);
    metaLayout->addWidget(m_descriptionLabel);
    metaLayout->addWidget(m_torrentSelector);
    metaLayout->addWidget(m_playButton);

    infoLayout->addLayout(metaLayout, 1);
    mainLayout->addLayout(infoLayout, 0);

    // -----------------
    // Reproductor MPV 
    // -----------------
    m_videoPlayer = new VideoPlayerWindow(container);
    m_videoPlayer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoPlayer->setMinimumHeight(420);

    m_videoPlayer->setStyleSheet(
        "VideoPlayerWindow {"
        "  border: 2px solid #89b4fa;"
        "  border-radius: 12px;"
        "  background-color: #11111b;"
        "}"
    );

    mainLayout->addWidget(m_videoPlayer, 1);

    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);
}

void MovieDetailWidget::setMovie(const MovieItem& movie)
{
    if (m_torrentEngine) m_torrentEngine->stop();
    if (m_videoPlayer) m_videoPlayer->stop();

    m_movie = movie;

    m_titleLabel->setText(movie.title);

    QString metaText = QString("Año: %1 | Valoración: %2 ⭐")
                           .arg(movie.releaseYear > 0 ? QString::number(movie.releaseYear) : "N/A")
                           .arg(movie.rating, 0, 'f', 1);
    m_metaLabel->setText(metaText);

    // --- Renderizar Tags de Géneros ---
    QLayoutItem* item;
    while ((item = m_genresLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    for (const QString& genre : movie.genres) {
        QLabel* tagLabel = new QLabel(genre, this);
        tagLabel->setStyleSheet(
            "QLabel {"
            "  background-color: #313244;"
            "  color: #cdd6f4;"
            "  border-radius: 10px;"
            "  padding: 3px 10px;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "}"
        );
        m_genresLayout->addWidget(tagLabel);
    }

    m_descriptionLabel->setText(movie.description.isEmpty() ? "Sin descripción disponible." : movie.description);

    if (!movie.poster.isEmpty()) {
        downloadPoster(movie.poster);
    } else {
        m_posterLabel->setText("Sin Póster");
    }

    m_torrentSelector->blockSignals(true);
    m_torrentSelector->clear();

    int torrentCount = 0;
    for (const auto& stream : m_movie.streams) {
        if (stream.url.startsWith("magnet:", Qt::CaseInsensitive)) {
            torrentCount++;
            QString lang = stream.language.isEmpty() ? "?" : stream.language;
            QString quality = stream.quality.isEmpty() ? "?" : stream.quality;

            QString label = QString("Calidad: [%1] - Idioma: [%2]").arg(quality, lang);
            m_torrentSelector->addItem(label, stream.url);
        }
    }
    
    m_torrentSelector->blockSignals(false);

    if (torrentCount == 0) {
        m_torrentSelector->setVisible(false);
        m_playButton->setEnabled(false);
        m_playButton->setText("Sin Torrents");
    } else {
        m_torrentSelector->setVisible(true);
        m_playButton->setEnabled(true);
        m_playButton->setText("Play");
    }
}

void MovieDetailWidget::downloadPoster(const QString& url)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pixmap;
            pixmap.loadFromData(reply->readAll());

            if (!pixmap.isNull()) {
                // Escalar manteniendo la relación de aspecto
                QPixmap scaled = pixmap.scaled(m_posterLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                m_posterLabel->setPixmap(scaled);
            } else {
                m_posterLabel->setText("Sin Imagen");
            }
        } else {
            m_posterLabel->setText("Error Poster");
        }
        reply->deleteLater();
    });
}

void MovieDetailWidget::startSelectedTorrent()
{
    if (m_torrentSelector->count() == 0) return;

    const QString magnetUrl = m_torrentSelector->currentData().toString();
    if (magnetUrl.isEmpty()) return;

    if (m_torrentEngine) m_torrentEngine->stop();
    if (m_videoPlayer) m_videoPlayer->stop();

    const QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString savePath = QDir(tempPath).filePath("iptv_torrents");

    m_playButton->setEnabled(false);
    m_playButton->setText("Buscando peers...");

    if (!m_torrentEngine->startMagnet(magnetUrl, savePath)) {
        m_playButton->setEnabled(true);
        m_playButton->setText("Play");
    }
}

void MovieDetailWidget::onTorrentReadyToPlay()
{
    if (!m_torrentEngine || !m_videoPlayer) return;

    const QString videoPath = m_torrentEngine->videoFilePath();
    if (videoPath.isEmpty()) return;

    m_playButton->setEnabled(false);
    m_playButton->setText("Reproduciendo...");

    m_videoPlayer->setTrackControlsVisible(true);
    m_videoPlayer->play(videoPath);
}

void MovieDetailWidget::onTorrentError(const QString& message)
{
    m_playButton->setEnabled(true);
    m_playButton->setText("Play");

    QMessageBox::warning(this, "Error de Reproducción", message);
}