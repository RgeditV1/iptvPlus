#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QTimer>
#include <memory>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>

class TorrentEngine : public QObject
{
    Q_OBJECT

public:
    explicit TorrentEngine(QObject* parent = nullptr);
    ~TorrentEngine();

    bool startMagnet(const QString& magnetUrl, const QString& savePath);
    void stop();

    quint16 httpPort() const { return m_serverPort; }
    QString streamUrl() const;

signals:
    void metadataLoaded(const QString& title, qint64 totalBytes);
    void progressUpdated(float progress, int downloadRate, int numPeers);
    void readyToPlay();
    void errorOccurred(const QString& message);

private slots:
    void onNewHttpConnection();
    void onHttpReadyRead();
    void updateEngineState();

private:
    void prioritizeSequentialPieces();
    int findLargestFileIndex(const lt::torrent_info& info);

    lt::session m_session;
    lt::torrent_handle m_handle;
    QTcpServer* m_httpServer{nullptr};
    QTcpSocket* m_clientSocket{nullptr};
    
    QTimer* m_statusTimer{nullptr};
    quint16 m_serverPort{8080};
    int m_videoFileIndex{-1};
    qint64 m_fileOffset{0};
    qint64 m_fileSize{0};
    bool m_isReadyToPlay{false};
};