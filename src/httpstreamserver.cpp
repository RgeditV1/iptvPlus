#include "httpstreamserver.hpp"
#include <QRegularExpression>
#include <QFileInfo>
#include <QDebug>

namespace {

QString getMimeType(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "mp4" || ext == "m4v") return "video/mp4";
    if (ext == "mkv")                return "video/x-matroska";
    if (ext == "avi")                return "video/x-msvideo";
    if (ext == "webm")               return "video/webm";
    if (ext == "mov")                return "video/quicktime";
    if (ext == "ts" || ext == "m2ts")return "video/mp2t";
    if (ext == "flv")                return "video/x-flv";
    if (ext == "wmv")                return "video/x-ms-wmv";
    return "application/octet-stream"; // Genérico para forzar lectura de stream
}

} // namespace

HttpStreamServer::HttpStreamServer(TorrentEngine* engine, QObject* parent)
    : QObject(parent), m_torrentEngine(engine)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &HttpStreamServer::handleNewConnection);
}

HttpStreamServer::~HttpStreamServer()
{
    stop();
}

bool HttpStreamServer::start(quint16 port)
{
    m_port = port;
    return m_tcpServer->listen(QHostAddress::LocalHost, m_port);
}

void HttpStreamServer::stop()
{
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

QString HttpStreamServer::streamUrl() const
{
    return QString("http://127.0.0.1:%1/stream").arg(m_port);
}

void HttpStreamServer::handleNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* socket = m_tcpServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            processClientRequest(socket);
        });
    }
}

void HttpStreamServer::processClientRequest(QTcpSocket* socket)
{
    QByteArray requestData = socket->readAll();
    QString requestStr = QString::fromUtf8(requestData);

    qint64 fileSize = m_torrentEngine->fileSize();
    if (fileSize <= 0) {
        socket->write("HTTP/1.1 503 Service Unavailable\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    qint64 startByte = 0;
    qint64 endByte = fileSize - 1;

    // Procesar cabecera HTTP Range
    QRegularExpression rangeRegex("Range: bytes=(\\d+)-(\\d*)");
    QRegularExpressionMatch match = rangeRegex.match(requestStr);

    if (match.hasMatch()) {
        startByte = match.captured(1).toLongLong();
        if (!match.captured(2).isEmpty()) {
            endByte = match.captured(2).toLongLong();
        }
    }

    qint64 contentLength = endByte - startByte + 1;

    // Repriorizar en libtorrent las piezas según el rango pedido (Seeking)
    m_torrentEngine->prioritizeRange(startByte, endByte);

    // Obtener MIME Type correcto en función del archivo real cargado
    QString mimeType = getMimeType(m_torrentEngine->videoFilePath());

    // Responder HTTP 206 Partial Content
    QByteArray header;
    header.append("HTTP/1.1 206 Partial Content\r\n");
    header.append(QString("Content-Type: %1\r\n").arg(mimeType).toUtf8());
    header.append(QString("Content-Length: %1\r\n").arg(contentLength).toUtf8());
    header.append(QString("Content-Range: bytes %1-%2/%3\r\n").arg(startByte).arg(endByte).arg(fileSize).toUtf8());
    header.append("Accept-Ranges: bytes\r\n");
    header.append("Connection: close\r\n\r\n");

    socket->write(header);

    qint64 currentOffset = startByte;
    constexpr qint64 chunkSize = 64 * 1024; // Transmisión en bloques de 64 KB

    while (currentOffset <= endByte && socket->state() == QAbstractSocket::ConnectedState) {
        qint64 bytesToRead = qMin(chunkSize, endByte - currentOffset + 1);
        QByteArray chunkData = m_torrentEngine->readBytesSynchronous(currentOffset, bytesToRead);

        if (chunkData.isEmpty()) {
            break; 
        }

        socket->write(chunkData);
        socket->waitForBytesWritten(100);
        currentOffset += chunkData.size();
    }

    socket->disconnectFromHost();
}