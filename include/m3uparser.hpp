#ifndef M3UPARSER_HPP
#define M3UPARSER_HPP

#include <QString>
#include <QList>
#include <QDir>

struct M3UItem {
    QString tvgId;
    QString title;
    QString url;
};

class M3UParser {
public:
    M3UParser() = default;

    QList<M3UItem> parseFile(const QString& filePath);

    QList<M3UItem> parseDirectory(const QString& directoryPath, bool recursive = true);
};

#endif // M3UPARSER_HPP