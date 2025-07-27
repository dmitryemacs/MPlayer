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
