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
#include <QEvent>

#include "databasemanager.hpp"

// Permite empaquetar MovieItem dentro de QVariant para el eventFilter
Q_DECLARE_METATYPE(MovieItem)

class MoviesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MoviesWidget(QWidget* parent = nullptr);
    ~MoviesWidget() override;

    void loadMoviesFromDatabase();

signals:
    void movieSelected(const MovieItem& movie);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSearchClicked();
    void onScrapFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void downloadPoster(const QString& url, QLabel* imageLabel);

private:
    void setupUi();
    void clearDB();

    QWidget* createMovieCard(const MovieItem& movie);

    QString m_currentSearchQuery;

    QLineEdit* m_searchLineEdit{nullptr};
    QPushButton* m_searchButton{nullptr};
    QProgressBar* m_loadingBar{nullptr};
    QScrollArea* m_scrollArea{nullptr};
    QWidget* m_gridContainer{nullptr};
    QGridLayout* m_gridLayout{nullptr};

    QProcess* m_scrapProcess{nullptr};
    QNetworkAccessManager* m_networkManager{nullptr};
};