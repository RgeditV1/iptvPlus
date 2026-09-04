#include "moviedetailswidget.hpp"
#include "dbmanager.hpp"

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

    QHBoxLayout* topNavLayout = new QHBoxLayout();
    topNavLayout->setContentsMargins(36, 0, 0, 0);
    topNavLayout->setSpacing(10);

    btnBack = new QPushButton("← Atras", this);
    btnBack->setObjectName("btnBack");
    btnBack->setCursor(Qt::PointingHandCursor);
    connect(btnBack, &QPushButton::clicked, this, &MovieDetailsWidget::backRequested);

    btnTrailer = new QPushButton("▶ Ver Tráiler", this);
    btnTrailer->setObjectName("btnTrailer");
    btnTrailer->setCursor(Qt::PointingHandCursor);
    btnTrailer->hide();
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

    // Contenedor horizontal de la ficha
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

    // Sinopsis
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

    // Servidores
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

    detailsLayout->addWidget(posterLabel, 0, Qt::AlignTop | Qt::AlignLeft);
    detailsLayout->addLayout(infoLayout, 1);

    mainLayout->addLayout(detailsLayout, 1);
}

void MovieDetailsWidget::updatePosterPixmap() {
    if (!currentPosterPixmap.isNull() && posterLabel) {
        posterLabel->setPixmap(currentPosterPixmap.scaled(
            posterLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }
}

void MovieDetailsWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updatePosterPixmap();
}

void MovieDetailsWidget::loadMovie(int mediaId) {
    QVariantMap movie = DbManager::instance().getMovieDetails(mediaId);

    titleLabel->setText(movie["title"].toString());

    double rating = movie["rating"].toDouble();
    if (rating > 0) {
        ratingLabel->setText(QString("★ %1 / 10").arg(rating, 0, 'f', 1));
        ratingLabel->show();
    } else {
        ratingLabel->hide();
    }

    QString genres = movie["genres"].toString();
    if (!genres.isEmpty()) {
        genresLabel->setText("Géneros: " + genres);
        genresLabel->show();
    } else {
        genresLabel->hide();
    }

    QString desc = movie["description"].toString();
    if (!desc.isEmpty()) {
        descriptionLabel->setText(desc);
    } else {
        descriptionLabel->setText("Sin descripción disponible.");
    }

    currentTrailerUrl = movie["trailer"].toString();
    btnTrailer->setVisible(!currentTrailerUrl.isEmpty());

    posterLabel->clear();
    posterLabel->setText("Cargando...");
    currentPosterPixmap = QPixmap();

    QString posterUrl = movie["poster"].toString();
    if (!posterUrl.isEmpty()) {
        QNetworkRequest request((QUrl(posterUrl)));
        QNetworkReply* reply = networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                if (currentPosterPixmap.loadFromData(reply->readAll())) {
                    updatePosterPixmap();
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

    // Limpiar servidores antiguos
    QLayoutItem* item;
    while ((item = serversLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

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