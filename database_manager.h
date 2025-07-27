#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QList>

struct SongInfo {
    int id;
    QString title;
    QString artist;
    QString album;
    QString filePath;
    int durationMs;
};

struct PlaylistInfo {
    int id;
    QString name;
};

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool connectToDatabase(const QString& hostName, int port,
                           const QString& dbName, const QString& userName,
                           const QString& password);
    void disconnectFromDatabase();
    bool createTables();

    QList<SongInfo> loadSongs();
    int addSong(const QString &filePath, const QString &title, const QString &artist,
                const QString &album, int durationMs);
    bool deleteSong(int songId);

    QList<PlaylistInfo> loadPlaylists();
    int createPlaylist(const QString &name);
    // Дополнительные методы для работы с плейлистами (добавление/удаление песен из плейлиста)
    // bool addSongToPlaylist(int playlistId, int songId, int order);
    // bool removeSongFromPlaylist(int playlistId, int songId);

private:
    QSqlDatabase db;
};

#endif // DATABASE_MANAGER_H
