#include "dbmanager.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QSqlError>
#include <QDebug>

DbManager& DbManager::instance() {
    static DbManager instance;
    return instance;
}

DbManager::DbManager() {
    initDatabase();
}

DbManager::~DbManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DbManager::initDatabase() {
    QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString appFolder = localAppData + "/iptvPlus";

    QDir dir(appFolder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString dbPath = appFolder + "/database.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "[DbManager] Error abriendo la base de datos SQLite:" << m_db.lastError().text();
        return false;
    }

    qDebug() << "[DbManager] Base de datos conectada con éxito en:" << dbPath;

    // Crear tablas básicas si no existen
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS movies ("
               "id INTEGER PRIMARY KEY,"
               "title TEXT,"
               "poster TEXT"
               ");");

    query.exec("CREATE TABLE IF NOT EXISTS streams ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "media_id INTEGER,"
               "server TEXT,"
               "url TEXT"
               ");");

    return true;
}

QList<QVariantMap> DbManager::getMovies(int limit) {
    QList<QVariantMap> movies;
    if (!m_db.isOpen()) {
        qWarning() << "[DbManager] No se pueden obtener películas, la BD no está abierta.";
        return movies;
    }

    QSqlQuery query;
    query.prepare("SELECT id, title, poster, url FROM media WHERE type = 'movie' ORDER BY id DESC LIMIT :limit;");
    query.bindValue(":limit", limit);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap movie;
            movie["id"] = query.value("id").toInt();
            movie["title"] = query.value("title").toString();
            movie["poster"] = query.value("poster").toString();
            movies.append(movie);
        }
    } else {
        qWarning() << "[DbManager] Error en la consulta de películas:" << query.lastError().text();
    }

    return movies;
}

QVariantMap DbManager::getMovieDetails(int mediaId) {
    QVariantMap result;
    if (!m_db.isOpen()) return result;

    // Obtener metadatos completos de la película
    QSqlQuery queryMovie;
    queryMovie.prepare("SELECT id, title, poster, description, rating, trailer FROM media WHERE id = :id LIMIT 1;");
    queryMovie.bindValue(":id", mediaId);

    if (queryMovie.exec() && queryMovie.next()) {
        result["id"] = queryMovie.value("id").toInt();
        result["title"] = queryMovie.value("title").toString();
        result["poster"] = queryMovie.value("poster").toString();
        result["description"] = queryMovie.value("description").toString();
        result["rating"] = queryMovie.value("rating").toDouble();
        result["trailer"] = queryMovie.value("trailer").toString();
    } else {
        return result;
    }

    // Obtener géneros asociados
    QStringList genresList;
    QSqlQuery queryGenres;
    queryGenres.prepare(
        "SELECT g.name FROM genres g "
        "JOIN media_genres mg ON g.id = mg.genre_id "
        "WHERE mg.media_id = :id;"
    );
    queryGenres.bindValue(":id", mediaId);
    if (queryGenres.exec()) {
        while (queryGenres.next()) {
            genresList.append(queryGenres.value("name").toString());
        }
    }
    result["genres"] = genresList.join(", ");

    // Obtener servidores de reproducción
    QList<QVariantMap> servers;
    QSqlQuery queryStreams;
    queryStreams.prepare("SELECT server, url FROM streams WHERE media_id = :id;");
    queryStreams.bindValue(":id", mediaId);

    if (queryStreams.exec()) {
        while (queryStreams.next()) {
            QVariantMap stream;
            stream["server"] = queryStreams.value("server").toString();
            stream["url"] = queryStreams.value("url").toString();
            servers.append(stream);
        }
    }
    result["servers"] = QVariant::fromValue(servers);

    return result;
}

QVariantMap DbManager::getMovieDetailsWithStream(int mediaId) {
    QVariantMap result;
    if (!m_db.isOpen()) return result;

    QSqlQuery query;
    query.prepare("SELECT media_id, server, url FROM streams WHERE media_id = :id LIMIT 1;");
    query.bindValue(":id", mediaId);

    if (query.exec() && query.next()) {
        result["server"] = query.value("server").toString();
        result["stream_url"] = query.value("url").toString();
    } else {
        qWarning() << "[DbManager] Error o sin resultados para el stream con media_id:" << mediaId;
    }

    return result;
}