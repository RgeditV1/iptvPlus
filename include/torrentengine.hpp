#pragma once

#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

#include <QFile>
#include <QObject>
#include <QTimer>
#include <QByteArray>

#include <memory>

class TorrentEngine final : public QObject
{
    Q_OBJECT

public:
    explicit TorrentEngine(QObject* parent = nullptr);
    ~TorrentEngine() override;

    bool startMagnet(const QString& magnetUrl, const QString& savePath);
    void stop();
    static void cleanTempDirectory();

    QString videoFilePath() const;
    qint64 fileSize() const;

    // Métodos para interacción con el servidor HTTP Stream
    void prioritizeRange(qint64 startByte, qint64 endByte);
    QByteArray readBytesSynchronous(qint64 offset, qint64 length);

signals:
    void metadataLoaded(const QString& fileName, qint64 fileSize);
    void progressUpdated(float progress, int downloadRate, int peers);
    void readyToPlay();
    void errorOccurred(const QString& message);

private slots:
    void updateEngineState();

private:
    int findVideoFileIndex(const libtorrent::torrent_info& info);
    bool initializeVideoFile();
    bool hasInitialVideoPieces() const;
    void prioritizeStreamingPieces();

    void processTorrentAlerts();

private:
    // Core libtorrent elements
    libtorrent::session m_session;
    libtorrent::torrent_handle m_handle;

    // Timers
    QTimer* m_statusTimer = nullptr;
    QTimer* m_timeoutTimer = nullptr;

    // Ruta de archivo
    QString m_videoFilePath;

    int m_videoFileIndex = -1;

    qint64 m_fileSize = 0;
    qint64 m_fileOffset = 0;

    // State flags
    bool m_isReadyToPlay = false;
    bool m_metadataLogged = false;

    libtorrent::piece_index_t m_lastRequestedPiece{-1};
};