#include "database_manager.h"
#include <QCryptographicHash> // Для хэширования паролей, если потребуется

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

    // НОВОЕ: Установка кодировки клиента для соединения с PostgreSQL
    db.setConnectOptions("client_encoding=UTF8");

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

    // Новые таблицы
    // Таблица Artists
    if (!query.exec("CREATE TABLE IF NOT EXISTS Artists ("
                    "id SERIAL PRIMARY KEY,"
                    "name VARCHAR(255) NOT NULL UNIQUE,"
                    "bio TEXT"
                    ");")) {
        qDebug() << "Ошибка создания таблицы Artists:" << query.lastError().text();
        success = false;
    }

    // Таблица Albums
    if (!query.exec("CREATE TABLE IF NOT EXISTS Albums ("
                    "id SERIAL PRIMARY KEY,"
                    "title VARCHAR(255) NOT NULL,"
                    "artist_id INTEGER REFERENCES Artists(id) ON DELETE SET NULL,"
                    "release_year INTEGER,"
                    "UNIQUE(title, artist_id)"
                    ");")) {
        qDebug() << "Ошибка создания таблицы Albums:" << query.lastError().text();
        success = false;
    }

    // Таблица Genres
    if (!query.exec("CREATE TABLE IF NOT EXISTS Genres ("
                    "id SERIAL PRIMARY KEY,"
                    "name VARCHAR(255) NOT NULL UNIQUE"
                    ");")) {
        qDebug() << "Ошибка создания таблицы Genres:" << query.lastError().text();
        success = false;
    }

    // Таблица SongGenres
    if (!query.exec("CREATE TABLE IF NOT EXISTS SongGenres ("
                    "song_id INTEGER REFERENCES Songs(id) ON DELETE CASCADE,"
                    "genre_id INTEGER REFERENCES Genres(id) ON DELETE CASCADE,"
                    "PRIMARY KEY (song_id, genre_id)"
                    ");")) {
        qDebug() << "Ошибка создания таблицы SongGenres:" << query.lastError().text();
        success = false;
    }

    // Таблица Users
    if (!query.exec("CREATE TABLE IF NOT EXISTS Users ("
                    "id SERIAL PRIMARY KEY,"
                    "username VARCHAR(255) NOT NULL UNIQUE,"
                    "password_hash TEXT NOT NULL,"
                    "email VARCHAR(255) UNIQUE"
                    ");")) {
        qDebug() << "Ошибка создания таблицы Users:" << query.lastError().text();
        success = false;
    }

    // Таблица PlaybackHistory
    if (!query.exec("CREATE TABLE IF NOT EXISTS PlaybackHistory ("
                    "id SERIAL PRIMARY KEY,"
                    "user_id INTEGER REFERENCES Users(id) ON DELETE CASCADE,"
                    "song_id INTEGER REFERENCES Songs(id) ON DELETE CASCADE,"
                    "played_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                    ");")) {
        qDebug() << "Ошибка создания таблицы PlaybackHistory:" << query.lastError().text();
        success = false;
    }

    return success;
}

// НОВОЕ: Реализация функции для заполнения БД начальными данными
bool DatabaseManager::seedDatabase()
{
    // Добавление пользователей (пароли должны быть хэшированы в реальном приложении)
    // Для примера используем простой хэш
    QString testUserPassHash = QString(QCryptographicHash::hash("testpass"_qba, QCryptographicHash::Sha256).toHex());
    QString adminPassHash = QString(QCryptographicHash::hash("adminpass"_qba, QCryptographicHash::Sha256).toHex());

    int testUserId = addUser("testuser", testUserPassHash, "test@example.com");
    int adminId = addUser("admin", adminPassHash, "admin@example.com");
    return true;
}

// --- Методы для Songs (существующие) ---
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

// --- Методы для Playlists (существующие и новые) ---
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

bool DatabaseManager::addSongToPlaylist(int playlistId, int songId, int songOrder)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO PlaylistSongs (playlist_id, song_id, song_order) "
                  "VALUES (:playlist_id, :song_id, :song_order) "
                  "ON CONFLICT (playlist_id, song_id) DO UPDATE SET song_order = EXCLUDED.song_order;");
    query.bindValue(":playlist_id", playlistId);
    query.bindValue(":song_id", songId);
    query.bindValue(":song_order", songOrder);
    if (!query.exec()) {
        qDebug() << "Ошибка при добавлении песни в плейлист:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<SongInfo> DatabaseManager::getSongsInPlaylist(int playlistId)
{
    QList<SongInfo> songs;
    QSqlQuery query(db);
    query.prepare("SELECT s.id, s.title, s.artist, s.album, s.file_path, s.duration_ms "
                  "FROM Songs s "
                  "JOIN PlaylistSongs ps ON s.id = ps.song_id "
                  "WHERE ps.playlist_id = :playlist_id "
                  "ORDER BY ps.song_order;");
    query.bindValue(":playlist_id", playlistId);
    if (query.exec()) {
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
        qDebug() << "Ошибка загрузки песен из плейлиста:" << query.lastError().text();
    }
    return songs;
}

bool DatabaseManager::removeSongFromPlaylist(int playlistId, int songId)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM PlaylistSongs WHERE playlist_id = :playlist_id AND song_id = :song_id;");
    query.bindValue(":playlist_id", playlistId);
    query.bindValue(":song_id", songId);
    if (!query.exec()) {
        qDebug() << "Ошибка при удалении песни из плейлиста:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DatabaseManager::deletePlaylist(int playlistId) // РЕАЛИЗАЦИЯ НОВОГО МЕТОДА
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM Playlists WHERE id = :id;");
    query.bindValue(":id", playlistId);

    if (query.exec()) {
        // Если удаление прошло успешно, возвращаем true, если была затронута хотя бы одна строка
        return query.numRowsAffected() > 0;
    } else {
        qDebug() << "Ошибка при удалении плейлиста из БД:" << query.lastError().text();
        return false;
    }
}


// --- Новые методы для Artists ---
int DatabaseManager::addArtist(const QString &name, const QString &bio)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO Artists (name, bio) VALUES (:name, :bio) "
                  "ON CONFLICT (name) DO UPDATE SET bio = EXCLUDED.bio RETURNING id;");
    query.bindValue(":name", name);
    query.bindValue(":bio", bio);
    if (query.exec()) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    } else {
        qDebug() << "Ошибка при добавлении исполнителя:" << query.lastError().text();
    }
    return -1;
}

QList<ArtistInfo> DatabaseManager::loadArtists()
{
    QList<ArtistInfo> artists;
    QSqlQuery query(db);
    if (query.exec("SELECT id, name, bio FROM Artists ORDER BY name")) {
        while (query.next()) {
            ArtistInfo artist;
            artist.id = query.value("id").toInt();
            artist.name = query.value("name").toString();
            artist.bio = query.value("bio").toString();
            artists.append(artist);
        }
    } else {
        qDebug() << "Ошибка загрузки исполнителей:" << query.lastError().text();
    }
    return artists;
}

int DatabaseManager::getArtistId(const QString &name)
{
    QSqlQuery query(db);
    query.prepare("SELECT id FROM Artists WHERE name = :name;");
    query.bindValue(":name", name);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    } else {
        // Если исполнитель не найден, добавляем его
        return addArtist(name);
    }
}

// --- Новые методы для Albums ---
int DatabaseManager::addAlbum(const QString &title, int artistId, int releaseYear)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO Albums (title, artist_id, release_year) VALUES (:title, :artist_id, :release_year) "
                  "ON CONFLICT (title, artist_id) DO UPDATE SET release_year = EXCLUDED.release_year RETURNING id;");
    query.bindValue(":title", title);
    query.bindValue(":artist_id", artistId);
    query.bindValue(":release_year", releaseYear);
    if (query.exec()) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    } else {
        qDebug() << "Ошибка при добавлении альбома:" << query.lastError().text();
    }
    return -1;
}

QList<AlbumInfo> DatabaseManager::loadAlbums()
{
    QList<AlbumInfo> albums;
    QSqlQuery query(db);
    if (query.exec("SELECT id, title, artist_id, release_year FROM Albums ORDER BY title")) {
        while (query.next()) {
            AlbumInfo album;
            album.id = query.value("id").toInt();
            album.title = query.value("title").toString();
            album.artistId = query.value("artist_id").toInt();
            album.releaseYear = query.value("release_year").toInt();
            albums.append(album);
        }
    } else {
        qDebug() << "Ошибка загрузки альбомов:" << query.lastError().text();
    }
    return albums;
}

int DatabaseManager::getAlbumId(const QString &title, int artistId)
{
    QSqlQuery query(db);
    query.prepare("SELECT id FROM Albums WHERE title = :title AND artist_id = :artist_id;");
    query.bindValue(":title", title);
    query.bindValue(":artist_id", artistId);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    } else {
        // Если альбом не найден, добавляем его
        return addAlbum(title, artistId);
    }
}

// --- Новые методы для Genres ---
int DatabaseManager::addGenre(const QString &name)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO Genres (name) VALUES (:name) ON CONFLICT (name) DO NOTHING RETURNING id;");
    query.bindValue(":name", name);
    if (query.exec()) {
        if (query.next()) {
            return query.value(0).toInt();
        } else {
            // Если конфликт произошел и ничего не было вставлено, значит запись уже существует.
            // Получим ID существующей записи.
            return getGenreId(name);
        }
    } else {
        qDebug() << "Ошибка при добавлении жанра:" << query.lastError().text();
    }
    return -1;
}

QList<GenreInfo> DatabaseManager::loadGenres()
{
    QList<GenreInfo> genres;
    QSqlQuery query(db);
    if (query.exec("SELECT id, name FROM Genres ORDER BY name")) {
        while (query.next()) {
            GenreInfo genre;
            genre.id = query.value("id").toInt();
            genre.name = query.value("name").toString();
            genres.append(genre);
        }
    } else {
        qDebug() << "Ошибка загрузки жанров:" << query.lastError().text();
    }
    return genres;
}

int DatabaseManager::getGenreId(const QString &name)
{
    QSqlQuery query(db);
    query.prepare("SELECT id FROM Genres WHERE name = :name;");
    query.bindValue(":name", name);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    } else {
        // Если жанр не найден, добавляем его
        return addGenre(name);
    }
}

// --- Новые методы для SongGenres ---
bool DatabaseManager::addSongGenre(int songId, int genreId)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO SongGenres (song_id, genre_id) VALUES (:song_id, :genre_id) ON CONFLICT (song_id, genre_id) DO NOTHING;");
    query.bindValue(":song_id", songId);
    query.bindValue(":genre_id", genreId);
    if (!query.exec()) {
        qDebug() << "Ошибка при привязке жанра к песне:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<GenreInfo> DatabaseManager::getGenresForSong(int songId)
{
    QList<GenreInfo> genres;
    QSqlQuery query(db);
    query.prepare("SELECT g.id, g.name FROM Genres g JOIN SongGenres sg ON g.id = sg.genre_id WHERE sg.song_id = :song_id;");
    query.bindValue(":song_id", songId);
    if (query.exec()) {
        while (query.next()) {
            GenreInfo genre;
            genre.id = query.value("id").toInt();
            genre.name = query.value("name").toString();
            genres.append(genre);
        }
    } else {
        qDebug() << "Ошибка загрузки жанров для песни:" << query.lastError().text();
    }
    return genres;
}

QList<SongInfo> DatabaseManager::getSongsForGenre(int genreId)
{
    QList<SongInfo> songs;
    QSqlQuery query(db);
    query.prepare("SELECT s.id, s.title, s.artist, s.album, s.file_path, s.duration_ms "
                  "FROM Songs s JOIN SongGenres sg ON s.id = sg.song_id WHERE sg.genre_id = :genre_id;");
    query.bindValue(":genre_id", genreId);
    if (query.exec()) {
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
        qDebug() << "Ошибка загрузки песен для жанра:" << query.lastError().text();
    }
    return songs;
}

// --- Новые методы для Users ---
int DatabaseManager::addUser(const QString &username, const QString &passwordHash, const QString &email)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO Users (username, password_hash, email) VALUES (:username, :password_hash, :email) "
                  "ON CONFLICT (username) DO NOTHING RETURNING id;"); // Предполагаем, что username уникален
    query.bindValue(":username", username);
    query.bindValue(":password_hash", passwordHash); // Хэш пароля должен быть передан извне
    query.bindValue(":email", email);
    if (query.exec()) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    } else {
        qDebug() << "Ошибка при добавлении пользователя:" << query.lastError().text();
    }
    return -1;
}

UserInfo DatabaseManager::getUser(const QString &username)
{
    UserInfo user;
    user.id = -1; // Устанавливаем невалидный ID по умолчанию
    QSqlQuery query(db);
    query.prepare("SELECT id, username, email FROM Users WHERE username = :username;");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        user.id = query.value("id").toInt();
        user.username = query.value("username").toString();
        user.email = query.value("email").toString();
    } else {
        qDebug() << "Пользователь не найден или ошибка при загрузке пользователя:" << query.lastError().text();
    }
    return user;
}

bool DatabaseManager::verifyUser(const QString &username, const QString &passwordHash)
{
    QSqlQuery query(db);
    query.prepare("SELECT id FROM Users WHERE username = :username AND password_hash = :password_hash;");
    query.bindValue(":username", username);
    query.bindValue(":password_hash", passwordHash);
    if (query.exec() && query.next()) {
        return true; // Пользователь найден и пароль совпадает
    } else {
        qDebug() << "Ошибка верификации пользователя или неверные учетные данные:" << query.lastError().text();
        return false;
    }
}

// --- Новые методы для PlaybackHistory ---
bool DatabaseManager::addPlaybackEntry(int userId, int songId)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO PlaybackHistory (user_id, song_id, played_at) VALUES (:user_id, :song_id, CURRENT_TIMESTAMP);");
    query.bindValue(":user_id", userId);
    query.bindValue(":song_id", songId);
    if (!query.exec()) {
        qDebug() << "Ошибка при добавлении записи в историю прослушиваний:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<PlaybackEntryInfo> DatabaseManager::getPlaybackHistory(int userId, int limit)
{
    QList<PlaybackEntryInfo> history;
    QSqlQuery query(db);
    query.prepare("SELECT id, user_id, song_id, played_at FROM PlaybackHistory WHERE user_id = :user_id ORDER BY played_at DESC LIMIT :limit;");
    query.bindValue(":user_id", userId);
    query.bindValue(":limit", limit);
    if (query.exec()) {
        while (query.next()) {
            PlaybackEntryInfo entry;
            entry.id = query.value("id").toInt();
            entry.userId = query.value("user_id").toInt();
            entry.songId = query.value("song_id").toInt();
            entry.playedAt = query.value("played_at").toDateTime();
            history.append(entry);
        }
    } else {
        qDebug() << "Ошибка загрузки истории прослушиваний:" << query.lastError().text();
    }
    return history;
}
