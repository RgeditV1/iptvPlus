#pragma once

#include <QString>
#include <QList>

struct M3UItem {
    QString tvgId;
    QString title;
    QString url;
};

class M3UParser {
public:
    M3UParser() = default;

    QList<M3UItem> parseContent(const QString& content);
};