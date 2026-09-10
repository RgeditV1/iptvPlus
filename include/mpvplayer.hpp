#pragma once

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <qwindowdefs.h>

struct mpv_handle;

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

signals:
    void positionChanged(double position);
    void durationChanged(double duration);
    void pauseChanged(bool paused);
    void muteChanged(bool muted);

    void bufferingChanged(bool buffering);

    void playbackStarted();
    void playbackFinished();

    void playbackError(const QString& error);

private:
    static void wakeupCallback(void* context);

    void processEvents();

    void handleEvent(struct mpv_event* event);

    void observeProperties();

private:
    mpv_handle* m_mpv = nullptr;
};