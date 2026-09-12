#pragma once

#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

#include <QFile>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <memory>

class TorrentEngine final : public QObject
{
    Q_OBJECT

public:
    explicit TorrentEngine(QObject* parent = nullptr);
    ~TorrentEngine() override;

    bool startMagnet(const QString& magnetUrl, const QString& savePath);
    void stop();

    QString streamUrl() const;

signals:
    void metadataLoaded(const QString& fileName, qint64 fileSize);
    void progressUpdated(float progress, int downloadRate, int peers);
    void readyToPlay();
    void errorOccurred(const QString& message);

private slots:
    void updateEngineState();
    void onNewHttpConnection();
    void onHttpReadyRead();
    void sendNextStreamChunk();

private:
    int findVideoFileIndex(const libtorrent::torrent_info& info);
    bool initializeVideoFile();
    void prioritizeStreamingPieces();

    void processTorrentAlerts();
    void processReadPieceAlert(const libtorrent::read_piece_alert* alert);

    bool parseHttpRequest(const QByteArray& request);
    void sendHttpResponse(int statusCode,
                           const QByteArray& statusText,
                           const QByteArray& contentType = "text/plain",
                           qint64 contentLength = 0,
                           const QByteArray& extraHeaders = {});

    void startStreaming();
    void stopStreaming();
    bool isPieceInsideVideo(libtorrent::piece_index_t piece) const;

private:
    // Core libtorrent elements
    libtorrent::session m_session;
    libtorrent::torrent_handle m_handle;

    // Networking
    QTcpServer* m_httpServer = nullptr;
    QTcpSocket* m_clientSocket = nullptr;

    // Timers
    QTimer* m_statusTimer = nullptr;
    QTimer* m_streamTimer = nullptr;
    QTimer* m_timeoutTimer = nullptr;

    // Streaming & File handling
    QFile m_streamFile;

    quint16 m_serverPort = 0;
    int m_videoFileIndex = -1;

    qint64 m_fileSize = 0;
    qint64 m_fileOffset = 0;
    qint64 m_streamStart = 0;
    qint64 m_streamEnd = 0;
    qint64 m_streamPosition = 0;

    // State flags
    bool m_isReadyToPlay = false;
    bool m_streaming = false;
    bool m_httpHeadersSent = false;
    bool m_metadataLogged = false;

    libtorrent::piece_index_t m_lastRequestedPiece{-1};
    QByteArray m_httpRequestBuffer;
};