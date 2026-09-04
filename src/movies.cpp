#include "movies.hpp"
#include "dbmanager.hpp"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QCoreApplication>
#include <QDir>
#include <QResizeEvent>
#include <QDebug>

MoviesWidget::MoviesWidget(QWidget* parent)
    : QWidget(parent), scraperProcess(nullptr)
{
    setupUi();
    networkManager = new QNetworkAccessManager(this);
}

MoviesWidget::~MoviesWidget() {}

void MoviesWidget::setupUi()
{
    setStyleSheet(
        "QWidget {"
        "    background-color: #121214;"
        "    color: white;"
        "}"
        "QScrollArea {"
        "    border: none;"
        "}"
        "QProgressBar {"
        "    border: none;"
        "    background-color: #1a1a20;"
        "    height: 4px;"
        "    text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #007acc;"
        "}"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(6);

    // Barra de progreso / Spinner
    loadingBar = new QProgressBar(this);
    loadingBar->setRange(0, 0);
    loadingBar->setFixedHeight(4);
    loadingBar->hide();

    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #888888; font-size: 14px; margin: 20px;");
    statusLabel->hide();

    mainLayout->addWidget(loadingBar);
    mainLayout->addWidget(statusLabel);

    // Área de desplazamiento
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    gridContainer = new QWidget();
    gridContainer->setStyleSheet("background-color: transparent;");

    moviesLayout = new QGridLayout(gridContainer);
    moviesLayout->setContentsMargins(0, 0, 0, 0);
    moviesLayout->setSpacing(16);

    scrollArea->setWidget(gridContainer);
    mainLayout->addWidget(scrollArea, 1);
}

void MoviesWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    // Si está en pantalla completa o el ancho es mayor a 1100px, usar 5 columnas; en caso contrario, 4
    int newColumns = (isFullScreen() || width() >= 1100) ? 5 : 4;

    if (newColumns != columnsCount) {
        columnsCount = newColumns;
        rearrangeGrid();
    }
}

void MoviesWidget::rearrangeGrid()
{
    QList<QWidget*> cards;

    // Extraer todos los widgets del grid actual
    for (int i = 0; i < moviesLayout->count(); ++i) {
        QLayoutItem* item = moviesLayout->itemAt(i);
        if (item && item->widget()) {
            cards.append(item->widget());
        }
    }

    // Volver a acomodarlos en la nueva cantidad de columnas
    for (int i = 0; i < cards.size(); ++i) {
        int row = i / columnsCount;
        int col = i % columnsCount;
        moviesLayout->addWidget(cards[i], row, col);
    }
}

void MoviesWidget::clearGrid()
{
    QLayoutItem* child;
    while ((child = moviesLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
}

void MoviesWidget::loadMovies(const QString& searchQuery, const QString& genre, int limit)
{
    QList<QVariantMap> cachedMovies = DbManager::instance().getMovies(limit);

    if (!cachedMovies.isEmpty() && searchQuery.isEmpty() && (genre.isEmpty() || genre == "Todos los géneros")) {
        clearGrid();
        statusLabel->hide();
        for (int i = 0; i < cachedMovies.size(); ++i) {
            const auto& movie = cachedMovies[i];
            addMovieCard(
                movie["id"].toInt(),
                movie["title"].toString(),
                movie["poster"].toString(),
                i
            );
        }
        return;
    }

    if (scraperProcess && scraperProcess->state() != QProcess::NotRunning) {
        scraperProcess->disconnect();
        scraperProcess->kill();
        scraperProcess->waitForFinished(1000);
        scraperProcess->deleteLater();
        scraperProcess = nullptr;
    }

    clearGrid();
    loadingBar->show();
    statusLabel->setText("Buscando películas...");
    statusLabel->show();

    scraperProcess = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");
    env.insert("PYTHONUTF8", "1");
    scraperProcess->setProcessEnvironment(env);

    QString program = QCoreApplication::applicationDirPath() + "/scrap.exe";

    QStringList arguments;
    arguments << "--limit" << QString::number(limit);

    if (!searchQuery.isEmpty()) {
        arguments << "--search" << searchQuery;
    } else if (!genre.isEmpty() && genre != "Todos los géneros") {
        arguments << "--genre" << genre;
    } else {
        arguments << "--search" << "a";
    }

    connect(scraperProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qWarning() << "[MoviesWidget] Fallo al iniciar scrap.exe:" << error;
        loadingBar->hide();
        statusLabel->setText("Error ejecutando 'scrap.exe'. Verifica que exista junto al ejecutable.");
        statusLabel->show();
    });

    connect(scraperProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MoviesWidget::onScraperFinished);

    scraperProcess->start(program, arguments);
}

void MoviesWidget::onScraperFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    loadingBar->hide();

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        qWarning() << "[MoviesWidget] Scraper finalizado con error. ExitCode:" << exitCode;
        statusLabel->setText("No se encontraron resultados o el scraper falló.");
        statusLabel->show();
        
        if (scraperProcess) {
            scraperProcess->deleteLater();
            scraperProcess = nullptr;
        }
        return;
    }

    qDebug() << "[MoviesWidget] Scraper finalizado exitosamente. Actualizando catálogo desde SQLite...";
    
    if (scraperProcess) {
        scraperProcess->deleteLater();
        scraperProcess = nullptr;
    }

    QList<QVariantMap> freshMovies = DbManager::instance().getMovies(20);
    clearGrid();

    if (freshMovies.isEmpty()) {
        statusLabel->setText("No se encontraron películas en la base de datos.");
        statusLabel->show();
        return;
    }

    statusLabel->hide();
    for (int i = 0; i < freshMovies.size(); ++i) {
        const auto& movie = freshMovies[i];
        addMovieCard(
            movie["id"].toInt(),
            movie["title"].toString(),
            movie["poster"].toString(),
            i
        );
    }
}

void MoviesWidget::addMovieCard(int mediaId, const QString& title, const QString& posterUrl, int index)
{
    QWidget* card = new QWidget(gridContainer);
    card->setFixedWidth(160);
    card->setCursor(Qt::PointingHandCursor);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(6);

    // Contenedor de la Imagen / Portada
    QLabel* imageLabel = new QLabel(card);
    imageLabel->setFixedSize(160, 230);
    imageLabel->setStyleSheet("background-color: #1e1e24; border-radius: 6px;");
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setText("Cargando...");

    // Etiqueta para el Título
    QLabel* titleLabel = new QLabel(title, card);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet("font-size: 12px; color: #e0e0e0; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    cardLayout->addWidget(imageLabel);
    cardLayout->addWidget(titleLabel);

    // Botón overlay transparente para gestionar la interacción de clic
    QPushButton* clickOverlay = new QPushButton(card);
    clickOverlay->setGeometry(0, 0, 160, 270);
    clickOverlay->setStyleSheet("background: transparent; border: none;");

    connect(clickOverlay, &QPushButton::clicked, this, [this, mediaId]() {
        QVariantMap streamData = DbManager::instance().getMovieDetailsWithStream(mediaId);
        QString streamUrl = streamData["stream_url"].toString();

        if (!streamUrl.isEmpty()) {
            emit movieSelected(streamUrl);
        } else {
            qWarning() << "[MoviesWidget] Sin enlaces de reproducción válidos para media_id:" << mediaId;
        }
    });

    int row = index / columnsCount;
    int col = index % columnsCount;
    moviesLayout->addWidget(card, row, col);

    // Descarga asíncrona de la portada
    if (!posterUrl.isEmpty()) {
        QNetworkRequest request((QUrl(posterUrl)));
        QNetworkReply* reply = networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [imageLabel, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                if (!pixmap.isNull()) {
                    imageLabel->setPixmap(pixmap.scaled(
                        imageLabel->size(),
                        Qt::KeepAspectRatioByExpanding,
                        Qt::SmoothTransformation
                    ));
                } else {
                    imageLabel->setText("Sin Imagen");
                }
            } else {
                imageLabel->setText("Sin Imagen");
            }
            reply->deleteLater();
        });
    } else {
        imageLabel->setText("Sin Imagen");
    }
}