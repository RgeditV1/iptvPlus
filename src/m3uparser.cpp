#include "m3uparser.hpp"
#include <QTextStream>
#include <QIODevice>
#include <QRegularExpression>

QList<M3UItem> M3UParser::parseContent(const QString& content) {
    QList<M3UItem> items;
    QTextStream in(const_cast<QString*>(&content), QIODevice::ReadOnly);

    M3UItem currentItem;
    bool hasPendingItem = false;

    QRegularExpression extinfRegex(QStringLiteral("#EXTINF:-?\\d+.*tvg-id=\"([^\"]*)\".*?,\\s*(.*)"));

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("#EXTINF:")) {
            QRegularExpressionMatch match = extinfRegex.match(line);
            if (match.hasMatch()) {
                currentItem.tvgId = match.captured(1);
                currentItem.title = match.captured(2);
                hasPendingItem = true;
            }
            else {
                int commaIndex = line.lastIndexOf(',');
                if (commaIndex != -1) {
                    currentItem.title = line.mid(commaIndex + 1).trimmed();
                    hasPendingItem = true;
                }
            }
        }
        else if (hasPendingItem && !line.isEmpty() && !line.startsWith("#")) {
            currentItem.url = line;
            items.append(currentItem);
            currentItem = M3UItem();
            hasPendingItem = false;
        }
    }
    return items;
}