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

    /**
     * @brief Analiza el contenido de texto de una lista de reproducción M3U y extrae sus elementos.
     * @param content Cadena de texto con la estructura de un archivo M3U/M3U8.
     * @return Lista de estructuras M3UItem obtenidas del contenido.
     */
    QList<M3UItem> parseContent(const QString& content);
};