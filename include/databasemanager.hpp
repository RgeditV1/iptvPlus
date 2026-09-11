#pragma once

#include <QString>
#include <QList>
#include <QSqlDatabase>

struct StreamInfo {
    int id{0};
    int mediaId{0};
    QString server;
    QString url; // Magnet URL o stream
    QString language;
    QString quality;
};

struct MovieItem {
    int id{0};
    QString title;
    QString type;
    QString url;
    QString poster;
    QString description;
    int releaseYear{0};
    double rating{0.0};
    QString trailer;
    QStringList genres;
    QList<StreamInfo> streams;
};

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initDatabase();
    bool isConnected() const;

    
    QList<MovieItem> getSavedMovies(int limit = 50, int offset = 0);
    MovieItem getMovieById(int movieId);
    QList<StreamInfo> getStreamsForMedia(int mediaId, const QString& targetLang = QString());

private:
    DatabaseManager();
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QString getDatabasePath() const;

    QSqlDatabase m_db;
    bool m_connected{false};
};