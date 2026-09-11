#include "databasemanager.hpp"

#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

QString DatabaseManager::getDatabasePath() const
{
    // Obtiene %LOCALAPPDATA%/iptvPlus o ~/.local/share/iptvPlus
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir dir(basePath + "/iptvPlus");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absoluteFilePath("database.db");
}

bool DatabaseManager::initDatabase()
{
    if (QSqlDatabase::contains("qt_inv_connection")) {
        m_db = QSqlDatabase::database("qt_inv_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "qt_inv_connection");
    }

    const QString dbPath = getDatabasePath();
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Error abriendo la BD SQLite:" << m_db.lastError().text();
        m_connected = false;
        return false;
    }

    // Configuración alineada con db.py
    QSqlQuery query(m_db);
    query.exec("PRAGMA journal_mode = WAL;");
    query.exec("PRAGMA foreign_keys = ON;");

    m_connected = true;
    return true;
}

bool DatabaseManager::isConnected() const
{
    return m_connected && m_db.isOpen();
}

QList<MovieItem> DatabaseManager::getSavedMovies(int limit, int offset)
{
    QList<MovieItem> movies;
    if (!isConnected()) return movies;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, title, type, url, poster, description, release_year, rating, trailer 
        FROM media 
        WHERE type = 'movie' 
        ORDER BY created_at DESC 
        LIMIT :limit OFFSET :offset
    )");
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);

    if (!query.exec()) {
        qWarning() << "Error consultando películas:" << query.lastError().text();
        return movies;
    }

    while (query.next()) {
        MovieItem item;
        item.id = query.value("id").toInt();
        item.title = query.value("title").toString();
        item.type = query.value("type").toString();
        item.url = query.value("url").toString();
        item.poster = query.value("poster").toString();
        item.description = query.value("description").toString();
        item.releaseYear = query.value("release_year").toInt();
        item.rating = query.value("rating").toDouble();
        item.trailer = query.value("trailer").toString();

        // Obtener géneros asociados a la película
        QSqlQuery genreQuery(m_db);
        genreQuery.prepare(R"(
            SELECT g.name FROM genres g
            JOIN media_genres mg ON g.id = mg.genre_id
            WHERE mg.media_id = :media_id
        )");
        genreQuery.bindValue(":media_id", item.id);
        if (genreQuery.exec()) {
            while (genreQuery.next()) {
                item.genres.append(genreQuery.value("name").toString());
            }
        }

        // Obtener los streams (magnet URLs) asociados
        item.streams = getStreamsForMedia(item.id);

        movies.append(item);
    }

    return movies;
}

MovieItem DatabaseManager::getMovieById(int movieId)
{
    MovieItem item;
    if (!isConnected()) return item;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, title, type, url, poster, description, release_year, rating, trailer 
        FROM media 
        WHERE id = :id
    )");
    query.bindValue(":id", movieId);

    if (query.exec() && query.next()) {
        item.id = query.value("id").toInt();
        item.title = query.value("title").toString();
        item.type = query.value("type").toString();
        item.url = query.value("url").toString();
        item.poster = query.value("poster").toString();
        item.description = query.value("description").toString();
        item.releaseYear = query.value("release_year").toInt();
        item.rating = query.value("rating").toDouble();
        item.trailer = query.value("trailer").toString();

        item.streams = getStreamsForMedia(item.id);
    }

    return item;
}

QList<StreamInfo> DatabaseManager::getStreamsForMedia(int mediaId, const QString& targetLang)
{
    QList<StreamInfo> streams;
    if (!isConnected()) return streams;

    QString sql = R"(
        SELECT id, media_id, server, url, language, quality 
        FROM streams 
        WHERE media_id = :media_id
    )";

    if (!targetLang.isEmpty()) {
        sql += " AND UPPER(language) = UPPER(:target_lang)";
    }

    sql += R"(
        ORDER BY 
            CASE UPPER(language)
                WHEN 'LATINO' THEN 1
                WHEN 'DUAL/MULTI' THEN 2
                WHEN 'ESP' THEN 3
                WHEN 'SUB' THEN 4
                ELSE 5
            END, quality DESC
    )";

    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":media_id", mediaId);
    if (!targetLang.isEmpty()) {
        query.bindValue(":target_lang", targetLang);
    }

    if (query.exec()) {
        while (query.next()) {
            StreamInfo stream;
            stream.id = query.value("id").toInt();
            stream.mediaId = query.value("media_id").toInt();
            stream.server = query.value("server").toString();
            stream.url = query.value("url").toString();
            stream.language = query.value("language").toString();
            stream.quality = query.value("quality").toString();

            streams.append(stream);
        }
    }

    return streams;
}