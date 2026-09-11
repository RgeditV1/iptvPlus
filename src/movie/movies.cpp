#include "movies.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QPixmap>
#include <QMouseEvent>
#include <QDebug>

class MovieCardWidget : public QWidget {
public:
    MovieCardWidget(const MovieItem& movie, QWidget* parent = nullptr) 
        : QWidget(parent), m_movie(movie) {}

    const MovieItem& movie() const { return m_movie; }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked(m_movie);
        }
        QWidget::mousePressEvent(event);
    }

signals:
    void clicked(const MovieItem& movie);

private:
    MovieItem m_movie;
};

MoviesWidget::MoviesWidget(QWidget* parent)
    : QWidget(parent)
{
    m_scrapProcess = new QProcess(this);
    m_networkManager = new QNetworkAccessManager(this);

    connect(m_scrapProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray out = m_scrapProcess->readAllStandardOutput();
        qDebug().noquote() << "[scrap.exe STDOUT]:\n" << QString::fromUtf8(out).trimmed();
    });

    connect(m_scrapProcess, &QProcess::readyReadStandardError, this, [this]() {
        const QByteArray err = m_scrapProcess->readAllStandardError();
        qWarning().noquote() << "[scrap.exe STDERR]:\n" << QString::fromUtf8(err).trimmed();
    });

    connect(m_scrapProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MoviesWidget::onScrapFinished);

    setupUi();
    loadMoviesFromDatabase();
}

MoviesWidget::~MoviesWidget() = default;

void MoviesWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // ----------------------------------------
    // Barra de búsqueda
    // ----------------------------------------
    QHBoxLayout* searchLayout = new QHBoxLayout();
    
    m_searchLineEdit = new QLineEdit(this);
    m_searchLineEdit->setPlaceholderText("Buscar película en línea...");
    m_searchLineEdit->setStyleSheet(
        "QLineEdit {"
        "   background-color: #1e1e2e;"
        "   color: #cdd6f4;"
        "   border: 1px solid #45475a;"
        "   border-radius: 6px;"
        "   padding: 8px 12px;"
        "   font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid #89b4fa;"
        "}"
    );

    m_searchButton = new QPushButton("Buscar", this);
    m_searchButton->setCursor(Qt::PointingHandCursor);
    m_searchButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #89b4fa;"
        "   color: #11111b;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 6px;"
        "   padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #b4befe;"
        "}"
    );

    searchLayout->addWidget(m_searchLineEdit, 1);
    searchLayout->addWidget(m_searchButton);
    mainLayout->addLayout(searchLayout);

    connect(m_searchLineEdit, &QLineEdit::returnPressed, this, &MoviesWidget::onSearchClicked);
    connect(m_searchButton, &QPushButton::clicked, this, &MoviesWidget::onSearchClicked);

    // Barra de progreso indicadora de scraping
    m_loadingBar = new QProgressBar(this);
    m_loadingBar->setRange(0, 0);
    m_loadingBar->setFixedHeight(4);
    m_loadingBar->setTextVisible(false);
    m_loadingBar->setStyleSheet(
        "QProgressBar {"
        "   border: none;"
        "   background-color: transparent;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #89b4fa;"
        "}"
    );
    m_loadingBar->hide();
    mainLayout->addWidget(m_loadingBar);

    // ----------------------------------------
    // Área de catálogo con Scroll
    // ----------------------------------------
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    m_gridContainer = new QWidget(m_scrollArea);
    m_gridContainer->setStyleSheet("background-color: transparent;");

    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);
    m_gridLayout->setSpacing(15);

    m_scrollArea->setWidget(m_gridContainer);
    mainLayout->addWidget(m_scrollArea, 1);
}

void MoviesWidget::onSearchClicked()
{
    const QString query = m_searchLineEdit->text().trimmed();
    if (query.isEmpty()) return;

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString scrapPath = QDir(appDir).filePath("scrap.exe");

    if (!QFileInfo::exists(scrapPath)) {
        qWarning() << "[MoviesWidget] ERROR: No se encontró scrap.exe en:" << scrapPath;
        return;
    }

    if (m_scrapProcess->state() != QProcess::NotRunning) {
        m_scrapProcess->kill();
        m_scrapProcess->waitForFinished();
    }

    m_loadingBar->show();
    m_searchButton->setEnabled(false);

    QStringList args;
    args << "--search" << query
         << "--limit" << "20"
         << "--save";

    qDebug() << "[MoviesWidget] Ejecutando:" << scrapPath << args;

    // Conectar captura de errores directos del sistema operativo
    connect(m_scrapProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qWarning() << "[MoviesWidget] Error al ejecutar scrap.exe:" << error;
        m_loadingBar->hide();
        m_searchButton->setEnabled(true);
    });

    m_scrapProcess->setProgram(scrapPath);
    m_scrapProcess->setArguments(args);
    m_scrapProcess->start();
}

void MoviesWidget::onScrapFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    m_loadingBar->hide();
    m_searchButton->setEnabled(true);

    // Recargar las películas de SQLite una vez finalizado el scraping
    loadMoviesFromDatabase();
}

void MoviesWidget::loadMoviesFromDatabase()
{
    // Limpiar grid anterior
    QLayoutItem* child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    // Cargar como máximo 20 películas
    const auto movies = DatabaseManager::instance().getSavedMovies(20, 0);

    qDebug() << "[MoviesWidget] Películas cargadas desde SQLite:" << movies.size();

    int columns = 5;
    int row = 0;
    int col = 0;

    for (const auto& movie : movies) {
        QWidget* card = createMovieCard(movie);
        m_gridLayout->addWidget(card, row, col);

        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }
}

QWidget* MoviesWidget::createMovieCard(const MovieItem& movie)
{
    QWidget* card = new QWidget(m_gridContainer);
    card->setFixedWidth(160);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(
        "QWidget {"
        "   background-color: #181825;"
        "   border-radius: 8px;"
        "}"
        "QWidget:hover {"
        "   background-color: #313244;"
        "}"
    );

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(8, 8, 8, 8);
    cardLayout->setSpacing(6);

    // Poster
    QLabel* posterLabel = new QLabel(card);
    posterLabel->setFixedSize(144, 210);
    posterLabel->setStyleSheet("background-color: #11111b; border-radius: 6px;");
    posterLabel->setScaledContents(true);
    posterLabel->setAlignment(Qt::AlignCenter);

    if (!movie.poster.isEmpty()) {
        downloadPoster(movie.poster, posterLabel);
    } else {
        posterLabel->setText("Sin Póster");
    }

    // Título de la película
    QLabel* titleLabel = new QLabel(movie.title, card);
    titleLabel->setWordWrap(true);
    titleLabel->setMaximumWidth(144);
    titleLabel->setStyleSheet("color: #cdd6f4; font-weight: bold; font-size: 12px;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    cardLayout->addWidget(posterLabel);
    cardLayout->addWidget(titleLabel, 1);

    return card;
}

void MoviesWidget::downloadPoster(const QString& url, QLabel* imageLabel)
{
    if (url.isEmpty()) return;

    QNetworkRequest request((QUrl(url)));
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, imageLabel]() {
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pixmap;
            pixmap.loadFromData(reply->readAll());
            if (!pixmap.isNull()) {
                imageLabel->setPixmap(pixmap);
            } else {
                imageLabel->setText("Sin Imagen");
            }
        } else {
            qWarning() << "[MoviesWidget] Error descargando poster:" << reply->errorString();
            imageLabel->setText("Error Poster");
        }
        reply->deleteLater();
    });
}