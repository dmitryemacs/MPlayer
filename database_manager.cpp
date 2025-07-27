#include "database_manager.h"

DatabaseManager::DatabaseManager()
{
    db = QSqlDatabase::addDatabase("QPSQL"); // Можно сделать тип БД параметризуемым
}

DatabaseManager::~DatabaseManager()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool DatabaseManager::connectToDatabase(const QString& hostName, int port,
                                        const QString& dbName, const QString& userName,
                                        const QString& password)
{
    db.setHostName(hostName);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(userName);
    db.setPassword(password);

    if (!db.open()) {
        qDebug() << "Ошибка подключения к базе данных:" << db.lastError().text();
        return false;
    } else {
        qDebug() << "Успешное подключение к базе данных!";
        return true;
    }
}

void DatabaseManager::disconnectFromDatabase()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(db);
    bool success = true;

    // Таблица Songs
    if (!query.exec("CREATE TABLE IF NOT EXISTS Songs ("
                    "id SERIAL PRIMARY KEY,"
                    "title VARCHAR(255) NOT NULL,"
                    "artist VARCHAR(255),"
                    "album VARCHAR(255),"
                    "file_path TEXT NOT NULL UNIQUE,"
                    "duration_ms INTEGER"
                    ");")) {
        qDebug() << "Ошибка создания таблицы Songs:" << query.lastError().text();
        success = false;
    }

    // Таблица Playlists
    if (!query.exec("CREATE TABLE IF NOT EXISTS Playlists ("
                    "id SERIAL PRIMARY KEY,"
                    "name VARCHAR(255) NOT NULL UNIQUE"
                    ");")) {
        qDebug() << "Ошибка создания таблицы Playlists:" << query.lastError().text();
        success = false;
    }

    // Таблица PlaylistSongs
    if (!query.exec("CREATE TABLE IF NOT EXISTS PlaylistSongs ("
                    "playlist_id INTEGER REFERENCES Playlists(id) ON DELETE CASCADE,"
                    "song_id INTEGER REFERENCES Songs(id) ON DELETE CASCADE,"
                    "song_order INTEGER,"
                    "PRIMARY KEY (playlist_id, song_id)"
                    ");")) {
        qDebug() << "Ошибка создания таблицы PlaylistSongs:" << query.lastError().text();
        success = false;
    }
    return success;
}

QList<SongInfo> DatabaseManager::loadSongs()
{
    QList<SongInfo> songs;
    QSqlQuery query(db);
    if (query.exec("SELECT id, title, artist, album, file_path, duration_ms FROM Songs ORDER BY title")) {
        while (query.next()) {
            SongInfo song;
            song.id = query.value("id").toInt();
            song.title = query.value("title").toString();
            song.artist = query.value("artist").toString();
            song.album = query.value("album").toString();
            song.filePath = query.value("file_path").toString();
            song.durationMs = query.value("duration_ms").toInt();
            songs.append(song);
        }
    } else {
        qDebug() << "Ошибка загрузки песен из БД:" << query.lastError().text();
    }
    return songs;
}

int DatabaseManager::addSong(const QString &filePath, const QString &title,
                             const QString &artist, const QString &album, int durationMs)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO Songs (title, artist, album, file_path, duration_ms) "
                  "VALUES (:title, :artist, :album, :file_path, :duration_ms) "
                  "ON CONFLICT (file_path) DO UPDATE SET title = EXCLUDED.title, artist = EXCLUDED.artist, album = EXCLUDED.album, duration_ms = EXCLUDED.duration_ms "
                  "RETURNING id;");
    query.bindValue(":title", title);
    query.bindValue(":artist", artist);
    query.bindValue(":album", album);
    query.bindValue(":file_path", filePath);
    query.bindValue(":duration_ms", durationMs);

    if (query.exec()) {
        if (query.next()) {
            int songId = query.value(0).toInt();
            qDebug() << "Песня добавлена/обновлена в БД с ID:" << songId;
            return songId;
        }
    } else {
        qDebug() << "Ошибка при добавлении песни в БД:" << query.lastError().text();
    }
    return -1;
}

bool DatabaseManager::deleteSong(int songId)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM Songs WHERE id = :id;");
    query.bindValue(":id", songId);

    if (query.exec()) {
        return query.numRowsAffected() > 0;
    } else {
        qDebug() << "Ошибка при удалении песни из БД:" << query.lastError().text();
        return false;
    }
}

QList<PlaylistInfo> DatabaseManager::loadPlaylists()
{
    QList<PlaylistInfo> playlists;
    QSqlQuery query(db);
    if (query.exec("SELECT id, name FROM Playlists ORDER BY name")) {
        while (query.next()) {
            PlaylistInfo playlist;
            playlist.id = query.value("id").toInt();
            playlist.name = query.value("name").toString();
            playlists.append(playlist);
        }
    } else {
        qDebug() << "Ошибка загрузки плейлистов из БД:" << query.lastError().text();
    }
    return playlists;
}

int DatabaseManager::createPlaylist(const QString &name)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO Playlists (name) VALUES (:name) RETURNING id;");
    query.bindValue(":name", name);
    if (query.exec()) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    } else {
        qDebug() << "Ошибка при создании плейлиста в БД:" << query.lastError().text();
    }
    return -1;
}
