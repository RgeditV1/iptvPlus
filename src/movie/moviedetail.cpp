#include "moviedetail.hpp"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QPixmap>
#include <QStandardPaths>
#include <QNetworkRequest>

#include <algorithm>

MovieDetailWidget::MovieDetailWidget(QWidget* parent)
    : QWidget(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_torrentEngine = new TorrentEngine(this);

    setupUi();

    // Conexiones de las señales de TorrentEngine
    connect(m_torrentEngine, &TorrentEngine::readyToPlay, this, &MovieDetailWidget::onTorrentReadyToPlay);
    connect(m_torrentEngine, &TorrentEngine::errorOccurred, this, &MovieDetailWidget::onTorrentError);
    connect(m_torrentEngine, &TorrentEngine::progressUpdated, this,
            [this](float progress, int downloadRate, int numPeers) {
                // Actualizar información en el botón solo mientras se está preparando/descargando antes de reproducir
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
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // ------------------------------------------------
    // Botón volver
    // ------------------------------------------------
    QHBoxLayout* topBarLayout = new QHBoxLayout();

    m_backButton = new QPushButton("< Volver", this);
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
        qDebug() << "[MovieDetailWidget] Volviendo...";
        if (m_torrentEngine) {
            m_torrentEngine->stop();
        }
        if (m_videoPlayer) {
            m_videoPlayer->stop();
        }
        emit backRequested();
    });

    // ------------------------------------------------
    // Información película
    // ------------------------------------------------
    QHBoxLayout* infoLayout = new QHBoxLayout();
    infoLayout->setSpacing(20);

    m_posterLabel = new QLabel(this);
    m_posterLabel->setFixedSize(180, 270);
    m_posterLabel->setStyleSheet("background-color: #11111b; border-radius: 8px;");
    m_posterLabel->setScaledContents(true);
    m_posterLabel->setAlignment(Qt::AlignCenter);

    infoLayout->addWidget(m_posterLabel);

    QVBoxLayout* metaLayout = new QVBoxLayout();
    metaLayout->setSpacing(10);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("color: #ffffff; font-size: 24px; font-weight: bold;");
    m_titleLabel->setWordWrap(true);

    m_metaLabel = new QLabel(this);
    m_metaLabel->setStyleSheet("color: #89b4fa; font-size: 14px; font-weight: bold;");

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setStyleSheet("color: #a6adc8; font-size: 13px;");
    m_descriptionLabel->setWordWrap(true);

    m_torrentSelector = new QComboBox(this);
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
        "QComboBox::drop-down {"
        "  border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #181825;"
        "  color: #cdd6f4;"
        "  selection-background-color: #45475a;"
        "}"
    );

    m_playButton = new QPushButton("Reproducción Torrent", this);
    m_playButton->setCursor(Qt::PointingHandCursor);
    m_playButton->setFixedWidth(260);
    m_playButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #a6e3a1;"
        "  color: #11111b;"
        "  font-weight: bold;"
        "  font-size: 14px;"
        "  border: none;"
        "  padding: 10px;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #94e2d5;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #45475a;"
        "  color: #bac2de;"
        "}"
    );

    // leer la opción elegida en el QComboBox
    connect(m_playButton, &QPushButton::clicked, this, [this]() {
        if (m_torrentSelector->count() == 0) {
            qWarning() << "[MovieDetailWidget] No hay torrents seleccionables.";
            return;
        }

        // Obtener el magnet URL guardado en la propiedad de datos del combo
        const QString magnetUrl = m_torrentSelector->currentData().toString();

        if (magnetUrl.isEmpty()) {
            qWarning() << "[MovieDetailWidget] El Magnet seleccionado está vacío.";
            return;
        }

        const QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        const QString savePath = QDir(tempPath).filePath("iptv_torrents");

        qDebug() << "[MovieDetailWidget] Iniciando torrent seleccionado:" << m_torrentSelector->currentText();

        m_playButton->setEnabled(false);
        m_playButton->setText("Buscando peers...");

        if (!m_torrentEngine->startMagnet(magnetUrl, savePath)) {
            m_playButton->setEnabled(true);
            m_playButton->setText("Play");
        }
    });

    metaLayout->addWidget(m_titleLabel);
    metaLayout->addWidget(m_metaLabel);
    metaLayout->addWidget(m_descriptionLabel);
    metaLayout->addWidget(m_torrentSelector);
    metaLayout->addWidget(m_playButton);
    metaLayout->addStretch();

    infoLayout->addLayout(metaLayout, 1);
    mainLayout->addLayout(infoLayout);

    // ------------------------------------------------
    // Reproductor
    // ------------------------------------------------
    m_videoPlayer = new VideoPlayerWindow(this);
    m_videoPlayer->setMinimumHeight(400);
    m_videoPlayer->setMaximumHeight(600);
    m_videoPlayer->setStyleSheet(
        "VideoPlayerWindow {"
        "  border: 2px solid #89b4fa;"
        "  border-radius: 12px;"
        "  background-color: #11111b;"
        "}"
    );

    mainLayout->addWidget(m_videoPlayer, 0, Qt::AlignCenter);
}

void MovieDetailWidget::setMovie(const MovieItem& movie)
{
    if (m_torrentEngine) {
        m_torrentEngine->stop();
    }
    if (m_videoPlayer) {
        m_videoPlayer->stop();
    }

    m_movie = movie;

    m_titleLabel->setText(movie.title);

    QString metaText = QString("Año: %1 | Valoración: %2 ⭐")
                           .arg(movie.releaseYear > 0 ? QString::number(movie.releaseYear) : "N/A")
                           .arg(movie.rating, 0, 'f', 1);

    if (!movie.genres.isEmpty()) {
        metaText += " | " + movie.genres.join(", ");
    }

    m_metaLabel->setText(metaText);
    m_descriptionLabel->setText(movie.description.isEmpty() ? "Sin descripción disponible." : movie.description);

    if (!movie.poster.isEmpty()) {
        downloadPoster(movie.poster);
    } else {
        m_posterLabel->setText("Sin Póster");
    }

    m_torrentSelector->clear();

    int torrentCount = 0;
    for (const auto& stream : m_movie.streams) {
        if (stream.url.startsWith("magnet:", Qt::CaseInsensitive)) {
            torrentCount++;
            
            // Formatear texto descriptivo con Idioma y Calidad
            QString lang = stream.language.isEmpty() ? "?" : stream.language;
            QString quality = stream.quality.isEmpty() ? "?" : stream.quality;

            QString label = QString("Calidad:[%1] - Idioma:[%2]").arg(quality, lang);

            m_torrentSelector->addItem(label, stream.url);
        }
    }

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
                m_posterLabel->setPixmap(pixmap);
            } else {
                m_posterLabel->setText("Sin Imagen");
            }
        } else {
            m_posterLabel->setText("Error Poster");
        }
        reply->deleteLater();
    });
}

void MovieDetailWidget::onTorrentReadyToPlay()
{
    if (!m_torrentEngine || !m_videoPlayer) {
        return;
    }

    const QString videoPath = m_torrentEngine->videoFilePath();
    if (videoPath.isEmpty()) {
        qWarning() << "[MovieDetailWidget] videoFilePath está vacío.";
        return;
    }

    qDebug() << "[MovieDetailWidget] Torrent listo. Reproduciendo archivo local con MPV:" << videoPath;

    m_playButton->setEnabled(false);
    m_playButton->setText("Reproduciendo...");
    
    m_videoPlayer->play(videoPath);
}

void MovieDetailWidget::onTorrentError(const QString& message)
{
    qWarning() << "[MovieDetailWidget] Torrent Error:" << message;

    m_playButton->setEnabled(true);
    m_playButton->setText("Play");

    QMessageBox::warning(this, "Error de Reproducción", message);
}