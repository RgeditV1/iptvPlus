#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "torrentengine.hpp"

class HttpStreamServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpStreamServer(TorrentEngine* engine, QObject* parent = nullptr);
    ~HttpStreamServer() override;

    bool start(quint16 port = 8080);
    void stop();
    QString streamUrl() const;

private slots:
    void handleNewConnection();

private:
    void processClientRequest(QTcpSocket* socket);

    QTcpServer* m_tcpServer = nullptr;
    TorrentEngine* m_torrentEngine = nullptr;
    quint16 m_port = 8080;
};