#pragma once

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <memory>

#include "m3uparser.hpp"

class UpdateNotifier : public QObject {
    Q_OBJECT

public:

    explicit UpdateNotifier(QObject* parent = nullptr)
        : QObject(parent),
        networkManager(new QNetworkAccessManager(this))
    {
    }

    /**
     * @brief Inicia peticiones asíncronas para descargar las listas M3U remotas desde el repositorio de iptv-org.
     */
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

            auto pendingRequests = std::make_shared<int>(0);
            auto allChannels = std::make_shared<QList<M3UItem>>();

            for (const QJsonValue& val : filesArray) {
                QJsonObject fileObj = val.toObject();
                QString fileName = fileObj["name"].toString();
                QString downloadUrl = fileObj["download_url"].toString();

                if (fileName.endsWith(".m3u", Qt::CaseInsensitive) || fileName.endsWith(".m3u8", Qt::CaseInsensitive)) {
                    (*pendingRequests)++;

                    // Remover extensión .m3u / .m3u8 del "name" para usarlo como país/categoría fallback
                    QString countryName = fileName;
                    countryName.remove(QRegularExpression("\\.m3u8?$", QRegularExpression::CaseInsensitiveOption));
                    countryName = countryName.toUpper();

                    QNetworkRequest fileRequest((QUrl(downloadUrl)));
                    fileRequest.setHeader(QNetworkRequest::UserAgentHeader, "IPTV-Plus-App");
                    
                    QNetworkReply* fileReply = networkManager->get(fileRequest);

                    connect(fileReply, &QNetworkReply::finished, this, [this, fileReply, pendingRequests, allChannels, countryName]() {
                        fileReply->deleteLater();

                        if (fileReply->error() == QNetworkReply::NoError) {
                            QString m3uContent = QString::fromUtf8(fileReply->readAll());
                            M3UParser parser;
                            
                            // Se pasa countryName como valor predeterminado si no hay tvg-country o group-title
                            QList<M3UItem> parsedChannels = parser.parseContent(m3uContent, countryName);

                            allChannels->append(parsedChannels);
                        } else {
                            qWarning() << "[UpdateNotifier] Error al descargar lista para país" << countryName << ":" << fileReply->errorString();
                        }

                        (*pendingRequests)--;

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
    /**
     * @brief Señal emitida cuando finaliza la descarga y procesamiento de todos los canales remotos.
     * @param channels Lista consolidada de canales cargados en memoria.
     */
    void remoteChannelsLoaded(const QList<M3UItem>& channels);

private:
    QNetworkAccessManager* networkManager;
};