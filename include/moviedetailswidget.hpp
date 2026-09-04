#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QResizeEvent>

class MovieDetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit MovieDetailsWidget(QWidget* parent = nullptr);
    ~MovieDetailsWidget();

    void loadMovie(int mediaId);

signals:
    void backRequested();
    void playStreamRequested(const QString& streamUrl);

protected:
    void resizeEvent(QResizeEvent* event) override;

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
    QPixmap currentPosterPixmap;

    void setupUi();
    void updatePosterPixmap();
};