#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MoviesWidget : public QWidget {
    Q_OBJECT

public:
    explicit MoviesWidget(QWidget* parent = nullptr);
    ~MoviesWidget();

    void loadMovies(const QString& searchQuery = "", const QString& genre = "", int limit = 20);

signals:
    void movieSelected(const QString& streamUrl);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onScraperFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void clearGrid();

private:
    QScrollArea* scrollArea;
    QWidget* gridContainer;
    QGridLayout* moviesLayout;
    QProgressBar* loadingBar;
    QLabel* statusLabel;

    QProcess* scraperProcess;
    QNetworkAccessManager* networkManager;

    int columnsCount = 4; // Por defecto 4 columnas

    void setupUi();
    void addMovieCard(int mediaId, const QString& title, const QString& posterUrl, int index);
    void rearrangeGrid();
};