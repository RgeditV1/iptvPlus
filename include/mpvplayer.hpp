#pragma once

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <qwindowdefs.h>

struct mpv_handle;

struct TrackInfo {
    int id = -1;
    QString type;    // "audio" o "sub"
    QString title;
    QString lang;
    bool selected = false;
};

class MpvPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MpvPlayer(QObject* parent = nullptr);
    ~MpvPlayer() override;

    bool initialize(WId wid);
    bool setVideoOutput(WId wid);
    void shutdown();

    void play(const QString& url);
    void pause();
    void resume();
    void stop();
    void setMuted(bool muted);
    bool isMuted() const;

    void setVolume(double volume);
    double volume() const;

    void seek(double seconds);

    double position() const;
    double duration() const;
    bool isLive() const; // pelicula o envivo

    QList<TrackInfo> availableTracks() const;
    void setAudioTrack(int trackId);
    void setSubtitleTrack(int trackId); // Usar -1 o 0 ("no") para desactivar subtítulos

signals:
    void positionChanged(double position);
    void durationChanged(double duration);
    void pauseChanged(bool paused);
    void muteChanged(bool muted);

    void bufferingChanged(bool buffering);

    void playbackStarted();
    void playbackFinished();
    void tracksChanged();

    void playbackError(const QString& error);

private:
    static void wakeupCallback(void* context);

    void processEvents();

    void handleEvent(struct mpv_event* event);

    void observeProperties();

private:
    mpv_handle* m_mpv = nullptr;
};