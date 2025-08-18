CREATE DATABASE music_player_db WITH ENCODING 'UTF8' LC_COLLATE='ru_RU.UTF-8' LC_CTYPE='ru_RU.UTF-8' TEMPLATE=template0;

-- music_player_db
CREATE TABLE IF NOT EXISTS Songs (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    artist VARCHAR(255),
    album VARCHAR(255),
    file_path TEXT NOT NULL UNIQUE,
    duration_ms INTEGER
);

CREATE TABLE IF NOT EXISTS Playlists (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS PlaylistSongs (
    playlist_id INTEGER REFERENCES Playlists(id) ON DELETE CASCADE,
    song_id INTEGER REFERENCES Songs(id) ON DELETE CASCADE,
    song_order INTEGER,
    PRIMARY KEY (playlist_id, song_id)
);

-- Новые таблицы для расширения функционала
CREATE TABLE IF NOT EXISTS Artists (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    bio TEXT
);

CREATE TABLE IF NOT EXISTS Albums (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    artist_id INTEGER REFERENCES Artists(id) ON DELETE SET NULL,
    release_year INTEGER,
    UNIQUE(title, artist_id)
);

CREATE TABLE IF NOT EXISTS Genres (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS SongGenres (
    song_id INTEGER REFERENCES Songs(id) ON DELETE CASCADE,
    genre_id INTEGER REFERENCES Genres(id) ON DELETE CASCADE,
    PRIMARY KEY (song_id, genre_id)
);

CREATE TABLE IF NOT EXISTS Users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(255) NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    email VARCHAR(255) UNIQUE
);

CREATE TABLE IF NOT EXISTS PlaybackHistory (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES Users(id) ON DELETE CASCADE,
    song_id INTEGER REFERENCES Songs(id) ON DELETE CASCADE,
    played_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
