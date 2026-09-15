#include "m3uparser.hpp"
#include <QTextStream>
#include <QRegularExpression>

QList<M3UItem> M3UParser::parseContent(const QString& content, const QString& defaultCountry)
{
    QList<M3UItem> items;
    QTextStream stream(const_cast<QString*>(&content));
    
    M3UItem currentItem;
    bool hasPendingItem = false;

    static const QRegularExpression tvgIdRegex("tvg-id=\"([^\"]*)\"");
    static const QRegularExpression groupRegex("group-title=\"([^\"]*)\"");
    static const QRegularExpression countryRegex("tvg-country=\"([^\"]*)\"");
    
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        
        if (line.startsWith("#EXTINF:")) {
            currentItem = M3UItem();

            QRegularExpressionMatch countryMatch = countryRegex.match(line);
            if (countryMatch.hasMatch() && !countryMatch.captured(1).trimmed().isEmpty()) {
                currentItem.country = countryMatch.captured(1).trimmed().toUpper();
            } else {
                QRegularExpressionMatch groupMatch = groupRegex.match(line);
                if (groupMatch.hasMatch() && !groupMatch.captured(1).trimmed().isEmpty()) {
                    currentItem.country = groupMatch.captured(1).trimmed().toUpper();
                } else {
                    currentItem.country = defaultCountry.trimmed().toUpper();
                }
            }
            
            QRegularExpressionMatch match = tvgIdRegex.match(line);
            if (match.hasMatch()) {
                currentItem.tvgId = match.captured(1);
            }
            
            int commaIndex = line.lastIndexOf(',');
            if (commaIndex != -1) {
                currentItem.title = line.mid(commaIndex + 1).trimmed();
            } else {
                currentItem.title = "?";
            }
            
            hasPendingItem = true;
        } else if (!line.startsWith('#') && !line.isEmpty() && hasPendingItem) {
            currentItem.url = line;
            items.append(currentItem);
            hasPendingItem = false;
        }
    }
    
    return items;
}