#include "moviedetailswidget.hpp"
#include "dbmanager.hpp"

#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

MovieDetailsWidget::MovieDetailsWidget(QWidget* parent)
    : QWidget(parent)
{
    networkManager = new QNetworkAccessManager(this);
    setupUi();
}

MovieDetailsWidget::~MovieDetailsWidget() {}

void MovieDetailsWidget::setupUi() {
    setStyleSheet(
        "QWidget { background-color: #121214; color: white; }"
        "QPushButton#btnBack {"
        "    background-color: #2b2b36;"
        "    border: 1px solid #3a3a4c;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    color: white;"
        "}"
        "QPushButton#btnBack:hover { background-color: #3a3a4c; }"
        "QPushButton#btnTrailer {"
        "    background-color: #cc181e;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    font-weight: bold;"
        "    color: white;"
        "}"
        "QPushButton#btnTrailer:hover { background-color: #e62117; }"
        "QPushButton.serverBtn {"
        "    background-color: #007acc;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 10px 18px;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    color: white;"
        "}"
        "QPushButton.serverBtn:hover { background-color: #005999; }"
        "QScrollArea { border: none; }"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // Barra superior de navegación
    QHBoxLayout* topNavLayout = new QHBoxLayout();
    btnBack = new QPushButton("← Volver a películas", this);
    btnBack->setObjectName("btnBack");
    btnBack->setCursor(Qt::PointingHandCursor);
    connect(btnBack, &QPushButton::clicked, this, &MovieDetailsWidget::backRequested);

    btnTrailer = new QPushButton("▶ Ver Tráiler", this);
    btnTrailer->setObjectName("btnTrailer");
    btnTrailer->setCursor(Qt::PointingHandCursor);
    btnTrailer->hide(); // Se muestra solo si existe tráiler
    connect(btnTrailer, &QPushButton::clicked, this, [this]() {
        if (!currentTrailerUrl.isEmpty()) {
            QDesktopServices::openUrl(QUrl(currentTrailerUrl));
        }
    });

    topNavLayout->addWidget(btnBack);
    topNavLayout->addSpacing(10);
    topNavLayout->addWidget(btnTrailer);
    topNavLayout->addStretch(1);

    mainLayout->addLayout(topNavLayout);

    // Contenedor horizontal de la ficha (Poster + Información)
    QHBoxLayout* detailsLayout = new QHBoxLayout();
    detailsLayout->setSpacing(24);

    // Poster
    posterLabel = new QLabel(this);
    posterLabel->setFixedSize(220, 320);
    posterLabel->setStyleSheet("background-color: #1e1e24; border-radius: 8px;");
    posterLabel->setAlignment(Qt::AlignCenter);

    // Información técnica
    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(10);

    titleLabel = new QLabel(this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
    titleLabel->setWordWrap(true);

    ratingLabel = new QLabel(this);
    ratingLabel->setStyleSheet("font-size: 14px; color: #f1c40f; font-weight: bold;");

    genresLabel = new QLabel(this);
    genresLabel->setStyleSheet("font-size: 13px; color: #888888;");
    genresLabel->setWordWrap(true);

    // Sinopsis dentro de un ScrollArea
    QScrollArea* descScroll = new QScrollArea(this);
    descScroll->setWidgetResizable(true);
    descScroll->setMaximumHeight(120);

    descriptionLabel = new QLabel(this);
    descriptionLabel->setStyleSheet("font-size: 13px; color: #cccccc; line-height: 1.4;");
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    descScroll->setWidget(descriptionLabel);

    QLabel* serverHeaderLabel = new QLabel("Servidores disponibles:", this);
    serverHeaderLabel->setStyleSheet("font-size: 14px; color: #ffffff; font-weight: bold; margin-top: 10px;");

    // Contenedor para botones de servidores
    serversContainer = new QWidget(this);
    serversLayout = new QHBoxLayout(serversContainer);
    serversLayout->setContentsMargins(0, 0, 0, 0);
    serversLayout->setSpacing(12);
    serversLayout->setAlignment(Qt::AlignLeft);

    infoLayout->addWidget(titleLabel);
    infoLayout->addWidget(ratingLabel);
    infoLayout->addWidget(genresLabel);
    infoLayout->addWidget(descScroll);
    infoLayout->addWidget(serverHeaderLabel);
    infoLayout->addWidget(serversContainer);
    infoLayout->addStretch(1);

    detailsLayout->addWidget(posterLabel);
    detailsLayout->addLayout(infoLayout, 1);

    mainLayout->addLayout(detailsLayout, 1);
}

void MovieDetailsWidget::loadMovie(int mediaId) {
    QVariantMap movie = DbManager::instance().getMovieDetails(mediaId);

    // Título
    titleLabel->setText(movie["title"].toString());

    // Rating (Puntuación)
    double rating = movie["rating"].toDouble();
    if (rating > 0) {
        ratingLabel->setText(QString("★ %1 / 10").arg(rating, 0, 'f', 1));
        ratingLabel->show();
    } else {
        ratingLabel->hide();
    }

    // Géneros
    QString genres = movie["genres"].toString();
    if (!genres.isEmpty()) {
        genresLabel->setText("Géneros: " + genres);
        genresLabel->show();
    } else {
        genresLabel->hide();
    }

    // Sinopsis
    QString desc = movie["description"].toString();
    if (!desc.isEmpty()) {
        descriptionLabel->setText(desc);
    } else {
        descriptionLabel->setText("Sin descripción disponible.");
    }

    // Tráiler
    currentTrailerUrl = movie["trailer"].toString();
    btnTrailer->setVisible(!currentTrailerUrl.isEmpty());

    // Cargar Poster
    posterLabel->setText("Cargando...");
    QString posterUrl = movie["poster"].toString();
    if (!posterUrl.isEmpty()) {
        QNetworkRequest request((QUrl(posterUrl)));
        QNetworkReply* reply = networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                if (!pixmap.isNull()) {
                    posterLabel->setPixmap(pixmap.scaled(
                        posterLabel->size(),
                        Qt::KeepAspectRatioByExpanding,
                        Qt::SmoothTransformation
                    ));
                } else {
                    posterLabel->setText("Sin Imagen");
                }
            } else {
                posterLabel->setText("Sin Imagen");
            }
            reply->deleteLater();
        });
    } else {
        posterLabel->setText("Sin Imagen");
    }

    // Limpiar botones de servidores anteriores
    QLayoutItem* item;
    while ((item = serversLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Cargar Botones de Servidores (Voe, Vimeo, etc.)
    QList<QVariantMap> servers = movie["servers"].value<QList<QVariantMap>>();

    if (servers.isEmpty()) {
        QLabel* noServerLabel = new QLabel("No hay servidores de reproducción disponibles.", serversContainer);
        noServerLabel->setStyleSheet("color: #ff5555;");
        serversLayout->addWidget(noServerLabel);
        return;
    }

    int serverCount = qMin(servers.size(), 2);
    for (int i = 0; i < serverCount; ++i) {
        QString serverName = servers[i]["server"].toString();
        QString streamUrl = servers[i]["url"].toString();

        if (serverName.isEmpty() || serverName == "Unknown") {
            serverName = QString("Servidor %1").arg(i + 1);
        }

        QPushButton* btnServer = new QPushButton(serverName, serversContainer);
        btnServer->setProperty("class", "serverBtn");
        btnServer->setCursor(Qt::PointingHandCursor);

        connect(btnServer, &QPushButton::clicked, this, [this, streamUrl]() {
            emit playStreamRequested(streamUrl);
        });

        serversLayout->addWidget(btnServer);
    }
}