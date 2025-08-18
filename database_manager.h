#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QList>
#include <QDateTime> // Для PlaybackHistory

// Существующие структуры
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

// Новые структуры
struct ArtistInfo {
    int id;
    QString name;
    QString bio;
};

struct AlbumInfo {
    int id;
    QString title;
    int artistId; // ID исполнителя, если привязан
    int releaseYear;
};

struct GenreInfo {
    int id;
    QString name;
};

struct UserInfo {
    int id;
    QString username;
    QString email;
    // Пароль не храним здесь для безопасности, только хэш в БД
};

struct PlaybackEntryInfo {
    int id;
    int userId;
    int songId;
    QDateTime playedAt;
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
    bool seedDatabase(); // НОВОЕ: Объявление функции для заполнения БД начальными данными

    // Методы для Songs
    QList<SongInfo> loadSongs();
    int addSong(const QString &filePath, const QString &title, const QString &artist,
                const QString &album, int durationMs);
    bool deleteSong(int songId);

    // Методы для Playlists
    QList<PlaylistInfo> loadPlaylists();
    int createPlaylist(const QString &name);
    bool addSongToPlaylist(int playlistId, int songId, int songOrder);
    QList<SongInfo> getSongsInPlaylist(int playlistId);
    bool removeSongFromPlaylist(int playlistId, int songId);
    bool deletePlaylist(int playlistId); // НОВЫЙ МЕТОД

    // Новые методы для Artists
    int addArtist(const QString &name, const QString &bio = "");
    QList<ArtistInfo> loadArtists();
    int getArtistId(const QString &name);

    // Новые методы для Albums
    int addAlbum(const QString &title, int artistId, int releaseYear = 0);
    QList<AlbumInfo> loadAlbums();
    int getAlbumId(const QString &title, int artistId);

    // Новые методы для Genres
    int addGenre(const QString &name);
    QList<GenreInfo> loadGenres();
    int getGenreId(const QString &name);

    // Новые методы для SongGenres (связующая таблица)
    bool addSongGenre(int songId, int genreId);
    QList<GenreInfo> getGenresForSong(int songId);
    QList<SongInfo> getSongsForGenre(int genreId);

    // Новые методы для Users
    int addUser(const QString &username, const QString &passwordHash, const QString &email = "");
    UserInfo getUser(const QString &username);
    bool verifyUser(const QString &username, const QString &passwordHash);

    // Новые методы для PlaybackHistory
    bool addPlaybackEntry(int userId, int songId);
    QList<PlaybackEntryInfo> getPlaybackHistory(int userId, int limit = 100);

private:
    QSqlDatabase db;
};

#endif // DATABASE_MANAGER_H
