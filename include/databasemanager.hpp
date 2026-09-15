#pragma once

#include <QString>
#include <QList>
#include <QSqlDatabase>

struct StreamInfo {
    int id = -1;
    int mediaId = -1;
    QString server;
    QString url;
    QString language;
    QString quality;
};

struct MovieItem {
    int id = -1;
    QString title;
    QString type;
    QString url;
    QString poster;
    QString description;
    int releaseYear = 0;
    double rating = 0.0;
    QString trailer;            // Future Purpose
    QList<QString> genres;
    QList<StreamInfo> streams;
};

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initDatabase();
    bool isConnected() const;
    
    QList<MovieItem> getSavedMovies(int limit = 50, int offset = 0, const QString& searchTerm = "");
    MovieItem getMovieById(int movieId);
    QList<StreamInfo> getStreamsForMedia(int mediaId, const QString& targetLang = "");

private:
    DatabaseManager();
    ~DatabaseManager();

    QString getDatabasePath() const;

    QSqlDatabase m_db;
    bool m_connected = false;
};