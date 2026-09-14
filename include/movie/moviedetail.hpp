#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QComboBox>

#include "databasemanager.hpp"
#include "videoplayerwindow.hpp"
#include "torrentengine.hpp"

class MovieDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MovieDetailWidget(QWidget* parent = nullptr);
    ~MovieDetailWidget() override;

    void setMovie(const MovieItem& movie);
    VideoPlayerWindow* player() const { return m_videoPlayer; }

signals:
    void backRequested();

private slots:
    void onTorrentReadyToPlay();
    void onTorrentError(const QString& message);
    void downloadPoster(const QString& url);

private:
    void startSelectedTorrent(); // change magnet url

private:
    void setupUi();

    MovieItem m_movie;
    
    // UI Elements
    QPushButton* m_backButton{nullptr};
    QLabel* m_posterLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_metaLabel{nullptr};
    QLabel* m_descriptionLabel{nullptr};
    QPushButton* m_playButton{nullptr};
    QComboBox* m_torrentSelector = nullptr;
    
    // Components
    VideoPlayerWindow* m_videoPlayer{nullptr};
    TorrentEngine* m_torrentEngine{nullptr};
    QNetworkAccessManager* m_networkManager{nullptr};
};