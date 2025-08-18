// music_player.h
#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QMediaMetaData>
#include <QImage>
#include <QBuffer>
#include <QDebug>

class MusicPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MusicPlayer(QObject *parent = nullptr);
    ~MusicPlayer();

    void play();
    void pause();
    void stop();
    void shuffle();
    void repeat();
    void setSource(const QString& filePath);
    void setVolume(int value);
    void setPosition(qint64 position);

    QMediaPlayer::PlaybackState playbackState() const;
    qint64 position() const;
    qint64 duration() const;
    QUrl currentSource() const; // Ensure this is present and correct

signals:
    void playbackStateChanged(QMediaPlayer::PlaybackState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void metaDataChanged(const QString& title, const QString& artist, const QString& album, const QImage& albumArt); // ИЗМЕНЕНО: добавлено album
    void errorOccurred(const QString& errorMessage); // This signal takes a QString
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);

private slots:
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void handlePositionChanged(qint64 position);
    void handleDurationChanged(qint64 duration);
    void handleMetaDataChanged();
    // THIS IS THE CRITICAL LINE:
    void handleError(QMediaPlayer::Error error, const QString &errorString); // <-- MUST BE const QString &

private:
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
};

#endif // MUSIC_PLAYER_H
