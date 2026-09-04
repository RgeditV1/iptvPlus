#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MovieDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit MovieDetailsWidget(QWidget* parent = nullptr);
    ~MovieDetailsWidget();

    void loadMovie(int mediaId);

signals:
    void backRequested();
    void playStreamRequested(const QString& streamUrl);

private:
    QLabel* posterLabel;
    QLabel* titleLabel;
    QLabel* ratingLabel;
    QLabel* genresLabel;
    QLabel* descriptionLabel;
    
    QWidget* serversContainer;
    QHBoxLayout* serversLayout;
    
    QPushButton* btnBack;
    QPushButton* btnTrailer;

    QNetworkAccessManager* networkManager;
    QString currentTrailerUrl;

    void setupUi();
};