#pragma once

#include <QString>
#include <QList>

struct M3UItem {
    QString title;
    QString url;
    QString tvgId;
    QString country;
};

class M3UParser {
public:
    M3UParser() = default;

    /**
     * @brief Analiza el contenido de un archivo M3U/M3U8.
     * @param content Cadena con el contenido M3U.
     * @param defaultCountry País a utilizar si la etiqueta #EXTINF no especifica tvg-country ni group-title.
     */
    QList<M3UItem> parseContent(const QString& content, const QString& defaultCountry = "Sin categoría");
};