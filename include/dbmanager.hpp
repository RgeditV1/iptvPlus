#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantMap>
#include <QList>
#include <QString>

class DbManager {
public:
    static DbManager& instance();

    bool initDatabase();
    QList<QVariantMap> getMovies(int limit = 20);
    QVariantMap getMovieDetails(int mediaId);
    QVariantMap getMovieDetailsWithStream(int mediaId);

private:
    DbManager();
    ~DbManager();
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    QSqlDatabase m_db;
};