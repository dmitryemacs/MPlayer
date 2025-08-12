#include "music_player.h"
#include <QFileInfo>
#include <QBuffer>
#include <QImage>

MusicPlayer::MusicPlayer(QObject *parent)
    : QObject(parent)
{
    // Инициализация QMediaPlayer и QAudioOutput
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);

    // Соединение сигналов QMediaPlayer с внутренними слотами MusicPlayer
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &MusicPlayer::handlePlaybackStateChanged);
    connect(mediaPlayer, &QMediaPlayer::positionChanged,
            this, &MusicPlayer::handlePositionChanged);
    connect(mediaPlayer, &QMediaPlayer::durationChanged,
            this, &MusicPlayer::handleDurationChanged);
    connect(mediaPlayer, &QMediaPlayer::metaDataChanged,
            this, &MusicPlayer::handleMetaDataChanged);


    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &MusicPlayer::mediaStatusChanged);


    // Подключение сигнала errorOccurred с правильной перегрузкой
    // В Qt 6, errorOccurred имеет сигнатуру (QMediaPlayer::Error, const QString &)
    connect(mediaPlayer, QOverload<QMediaPlayer::Error, const QString &>::of(&QMediaPlayer::errorOccurred),
            this, &MusicPlayer::handleError);
}

MusicPlayer::~MusicPlayer()
{
    // QMediaPlayer и QAudioOutput будут удалены автоматически, так как
    // они имеют MusicPlayer как родительский объект.
    // Если бы они не имели родителя, их нужно было бы удалить вручную:
    // delete mediaPlayer;
    // delete audioOutput;
}

void MusicPlayer::play()
{
    mediaPlayer->play();
}

void MusicPlayer::pause()
{
    mediaPlayer->pause();
}

void MusicPlayer::repeat() {

    mediaPlayer->setLoops(3);
}

void MusicPlayer::stop()
{
    mediaPlayer->stop();
}

void MusicPlayer::setSource(const QString& filePath)
{
    // Устанавливает источник воспроизведения для плеера
    mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
}

void MusicPlayer::setVolume(int value) {
    // Устанавливает громкость (значение от 0 до 100)
    audioOutput->setVolume(value / 100.0);
}

void MusicPlayer::setPosition(qint64 position)
{
    // Устанавливает текущую позицию воспроизведения (в миллисекундах)
    mediaPlayer->setPosition(position);
}

QMediaPlayer::PlaybackState MusicPlayer::playbackState() const
{
    // Возвращает текущее состояние воспроизведения плеера
    return mediaPlayer->playbackState();
}

qint64 MusicPlayer::position() const
{
    // Возвращает текущую позицию воспроизведения (в миллисекундах)
    return mediaPlayer->position();
}

qint64 MusicPlayer::duration() const
{
    // Возвращает общую длительность текущего медиафайла (в миллисекундах)
    return mediaPlayer->duration();
}

QUrl MusicPlayer::currentSource() const
{
    // Возвращает URL текущего источника воспроизведения
    return mediaPlayer->source();
}

// --- Обработчики сигналов от QMediaPlayer ---

void MusicPlayer::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    // Переизлучает сигнал об изменении состояния воспроизведения
    emit playbackStateChanged(state);
}

void MusicPlayer::handlePositionChanged(qint64 position)
{
    // Переизлучает сигнал об изменении текущей позиции
    emit positionChanged(position);
}

void MusicPlayer::handleDurationChanged(qint64 duration)
{
    // Переизлучает сигнал об изменении длительности
    emit durationChanged(duration);
}

void MusicPlayer::handleMetaDataChanged()
{
    // Извлекает метаданные трека и обложку альбома, затем переизлучает сигнал
    QString title = mediaPlayer->metaData().stringValue(QMediaMetaData::Title);
    QString artist = mediaPlayer->metaData().stringValue(QMediaMetaData::AlbumArtist);
    if (artist.isEmpty()) {
        artist = mediaPlayer->metaData().stringValue(QMediaMetaData::ContributingArtist);
    }
    // QString album = mediaPlayer->metaData().stringValue(QMediaMetaData::AlbumTitle); // Можно использовать, если нужно

    // Если заголовок отсутствует в метаданных, используем базовое имя файла
    if (title.isEmpty()) {
        QFileInfo fileInfo(mediaPlayer->source().toLocalFile());
        title = fileInfo.baseName();
    }

    QImage coverImage;
    // Используем ThumbnailImage для обложки альбома (наиболее распространенное)
    QVariant imageData = mediaPlayer->metaData().value(QMediaMetaData::ThumbnailImage);

    if (imageData.isValid()) {
        if (imageData.canConvert<QImage>()) {
            coverImage = imageData.value<QImage>();
        } else if (imageData.canConvert<QByteArray>()) {
            QByteArray byteArray = imageData.toByteArray();
            QBuffer buffer(&byteArray); // Используем QBuffer для чтения QByteArray как файла
            buffer.open(QIODevice::ReadOnly);
            // Пробуем загрузить как JPG, затем как PNG
            if (!coverImage.load(&buffer, "JPG")) {
                buffer.seek(0); // Сброс позиции буфера
                if (!coverImage.load(&buffer, "PNG")) {
                    // Можно добавить обработку других форматов или сообщение об ошибке
                    qDebug() << "Не удалось загрузить обложку из QByteArray как JPG или PNG.";
                }
            }
            buffer.close();
        }
    }
    emit metaDataChanged(title, artist, coverImage);
}

void MusicPlayer::handleError(QMediaPlayer::Error error, const QString &errorString)
{
    // Обрабатывает ошибки QMediaPlayer и переизлучает их
    Q_UNUSED(error); // Если сам enum ошибки не используется, можно его игнорировать
    qDebug() << "Player error occurred: " << errorString;
    emit errorOccurred(errorString);
}
