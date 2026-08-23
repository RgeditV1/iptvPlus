#ifndef UPDATE_HPP
#define UPDATE_HPP

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QDebug>
#include "m3uparser.hpp"

class UpdateNotifier : public QObject {
    Q_OBJECT

public:
    explicit UpdateNotifier(QObject* parent = nullptr)
        : QObject(parent),
        networkManager(new QNetworkAccessManager(this))
    {
    }

    inline void fetchRemoteStreams() {
        qDebug() << "[UpdateNotifier] Obteniendo índice de archivos de iptv-org/iptv...";

        QUrl url("https://api.github.com/repos/iptv-org/iptv/contents/streams");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, "IPTV-Plus-App");

        QNetworkReply* reply = networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                qWarning() << "[UpdateNotifier] Error al obtener el directorio remoto:" << reply->errorString();
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isArray()) return;

            QJsonArray filesArray = doc.array();

            // Usamos punteros compartidos/estructuras para acumular las peticiones en curso
            auto pendingRequests = std::make_shared<int>(0);
            auto allChannels = std::make_shared<QList<M3UItem>>();

            for (const QJsonValue& val : filesArray) {
                QJsonObject fileObj = val.toObject();
                QString fileName = fileObj["name"].toString();
                QString downloadUrl = fileObj["download_url"].toString();

                if (fileName.endsWith(".m3u") || fileName.endsWith(".m3u8")) {
                    (*pendingRequests)++;

                    QNetworkRequest fileRequest((QUrl(downloadUrl)));
                    QNetworkReply* fileReply = networkManager->get(fileRequest);

                    connect(fileReply, &QNetworkReply::finished, this, [this, fileReply, pendingRequests, allChannels]() {
                        fileReply->deleteLater();

                        if (fileReply->error() == QNetworkReply::NoError) {
                            QString m3uContent = QString::fromUtf8(fileReply->readAll());
                            M3UParser parser;
                            allChannels->append(parser.parseContent(m3uContent));
                        }

                        (*pendingRequests)--;

                        // Emitir la señal con todos los canales procesados una vez terminadas todas las descargas
                        if (*pendingRequests == 0) {
                            qDebug() << "[UpdateNotifier] Descarga remota finalizada. Total de canales cargados:" << allChannels->size();
                            emit remoteChannelsLoaded(*allChannels);
                        }
                        });
                }
            }
            });
    }

signals:
    // Emite la lista completa de canales procesados en RAM
    void remoteChannelsLoaded(const QList<M3UItem>& channels);

private:
    QNetworkAccessManager* networkManager;
};

#endif // UPDATE_HPP