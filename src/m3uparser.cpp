#include "m3uparser.hpp"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDebug>

QList<M3UItem> M3UParser::parseFile(const QString& filePath) {
    QList<M3UItem> items;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[M3UParser] No se pudo abrir el archivo:" << filePath;
        return items;
    }

    QTextStream in(&file);
    M3UItem currentItem;
    bool hasPendingItem = false;

    // Captura opcionalmente tvg-id="..." y el título tras la última coma
    QRegularExpression extinfRegex(R"(#EXTINF:-?\d+\s*(?:tvg-id="([^"]*)") ? [^, ] *, ? \s * (.*))");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("#EXTINF:")) {
            QRegularExpressionMatch match = extinfRegex.match(line);
            if (match.hasMatch()) {
                currentItem.tvgId = match.captured(1);
                currentItem.title = match.captured(2);
                hasPendingItem = true;
            }
        }
        else if (hasPendingItem && !line.isEmpty() && !line.startsWith("#")) {
            currentItem.url = line;
            items.append(currentItem);
            currentItem = M3UItem();
            hasPendingItem = false;
        }
    }

    file.close();
    return items;
}

QList<M3UItem> M3UParser::parseDirectory(const QString& directoryPath, bool recursive) {
    QList<M3UItem> allItems;
    QDir dir(directoryPath);

    if (!dir.exists()) {
        qWarning() << "[M3UParser] El directorio no existe:" << directoryPath;
        return allItems;
    }

    QStringList filters;
    filters << "*.m3u" << "*.m3u8";

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) {
        flags = QDirIterator::Subdirectories;
    }

    QDirIterator it(directoryPath, filters, QDir::Files, flags);

    while (it.hasNext()) {
        QString filePath = it.next();
        allItems.append(parseFile(filePath));
    }

    qDebug() << "[M3UParser] Total de canales cargados desde" << directoryPath << ":" << allItems.size();
    return allItems;
}