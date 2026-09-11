#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QScrollArea>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "databasemanager.hpp"

class MoviesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MoviesWidget(QWidget* parent = nullptr);
    ~MoviesWidget();

    void loadMoviesFromDatabase();

signals:
    void movieSelected(const MovieItem& movie);

private slots:
    void onSearchClicked();
    void onScrapFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void downloadPoster(const QString& url, QLabel* imageLabel);

private:
    void setupUi();
    QWidget* createMovieCard(const MovieItem& movie);

    QLineEdit* m_searchLineEdit{nullptr};
    QPushButton* m_searchButton{nullptr};
    QProgressBar* m_loadingBar{nullptr};
    QScrollArea* m_scrollArea{nullptr};
    QWidget* m_gridContainer{nullptr};
    QGridLayout* m_gridLayout{nullptr};

    QProcess* m_scrapProcess{nullptr};
    QNetworkAccessManager* m_networkManager{nullptr};
};