#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    musicPlayer = new MusicPlayer(this);
    dbManager = new DatabaseManager();

    // Подключение к базе данных
    if (!dbManager->connectToDatabase("localhost", 5432, "music_player_db", "dima", "zxc011")) {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось подключиться к базе данных. Проверьте настройки.");
        return;
    }
    dbManager->createTables();

    // Инициализация моделей для QListView
    songListModel = new QStandardItemModel(this);
    ui->songListView->setModel(songListModel);

    playlistListModel = new QStandardItemModel(this);
    ui->playlistListView->setModel(playlistListModel);

    // Загрузка данных из БД при запуске
    loadDataIntoModels();

    // Начальное состояние UI
    initializeUIState();

    // Соединение сигналов от MusicPlayer со слотами MainWindow
    connect(musicPlayer, &MusicPlayer::playbackStateChanged, this, &MainWindow::handlePlayerPlaybackStateChanged);
    connect(musicPlayer, &MusicPlayer::positionChanged, this, &MainWindow::handlePlayerPositionChanged);
    connect(musicPlayer, &MusicPlayer::durationChanged, this, &MainWindow::handlePlayerDurationChanged);
    connect(musicPlayer, &MusicPlayer::metaDataChanged, this, &MainWindow::handlePlayerMetaDataChanged);
    connect(musicPlayer, &MusicPlayer::errorOccurred, this, &MainWindow::handlePlayerError);
    connect(musicPlayer, &MusicPlayer::mediaStatusChanged, this, &MainWindow::handleMediaStatusChanged);


    // Установка громкости по умолчанию
    musicPlayer->setVolume(ui->volumeSlider->value());
}

MainWindow::~MainWindow()
{
    delete ui;
    // dbManager и musicPlayer будут удалены автоматически, если они имеют MainWindow как родительский объект,
    // или должны быть удалены вручную, если они созданы без родителя.
    // В данном случае, dbManager создан без родителя, поэтому его нужно удалить вручную.
    delete dbManager;
}

void MainWindow::initializeUIState()
{
    ui->playButton->setEnabled(songListModel->rowCount() > 0);
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->nextButton->setEnabled(false);
    ui->previousButton->setEnabled(false);
    ui->progressBar->setEnabled(false);
    ui->deleteSongButton->setEnabled(songListModel->rowCount() > 0);
    ui->albumArtLabel->setText("No Album Art");
    ui->currentTrackLabel->setText("Нет трека");
    ui->currentTimeLabel->setText(formatTime(0));
    ui->totalTimeLabel->setText(formatTime(0));
    ui->progressBar->setValue(0);
}

void MainWindow::loadDataIntoModels()
{
    // Загрузка песен
    songListModel->clear();
    QList<SongInfo> songs = dbManager->loadSongs();
    for (const SongInfo& song : songs) {
        QStandardItem *item = new QStandardItem(song.title + " - " + song.artist);
        item->setData(song.id, Qt::UserRole + 1);      // Song ID
        item->setData(song.filePath, Qt::UserRole + 2); // File Path
        songListModel->appendRow(item);
    }

    // Загрузка плейлистов
    playlistListModel->clear();
    QList<PlaylistInfo> playlists = dbManager->loadPlaylists();
    for (const PlaylistInfo& playlist : playlists) {
        QStandardItem *item = new QStandardItem(playlist.name);
        item->setData(playlist.id, Qt::UserRole + 1);  // Playlist ID
        playlistListModel->appendRow(item);
    }
}

// --- Слоты для кнопок управления плеером ---
void MainWindow::on_playButton_clicked()
{
    if (musicPlayer->playbackState() == QMediaPlayer::PausedState ||
        musicPlayer->playbackState() == QMediaPlayer::StoppedState) {
        musicPlayer->play();
    }
}

void MainWindow::on_pauseButton_clicked()
{
    if (musicPlayer->playbackState() == QMediaPlayer::PlayingState) {
        musicPlayer->pause();
    }
}

void MainWindow::on_stopButton_clicked()
{
    musicPlayer->stop();
    ui->currentTrackLabel->setText("Нет трека");
    ui->currentTimeLabel->setText(formatTime(0));
    ui->totalTimeLabel->setText(formatTime(0));
    ui->progressBar->setValue(0);
    ui->progressBar->setEnabled(false);
    updateAlbumArt(QImage()); // Очистка обложки
    updateUIForPlaybackState(QMediaPlayer::StoppedState); // Обновление состояния кнопок
}

void MainWindow::on_nextButton_clicked()
{
    qDebug() << "Следующий трек (не реализовано)";
    // Логика перехода к следующему треку (возможно, из списка воспроизведения)
}

void MainWindow::on_previousButton_clicked()
{
    qDebug() << "Предыдущий трек (не реализовано)";
    // Логика перехода к предыдущему треку (возможно, из списка воспроизведения)
}

// --- Слоты для прогресс-бара и громкости ---
void MainWindow::on_progressBar_sliderMoved(int position)
{
    musicPlayer->setPosition(position);
}

void MainWindow::on_volumeSlider_valueChanged(int value)
{
    musicPlayer->setVolume(value);
}

// --- Слоты для добавления песен и плейлистов ---
void MainWindow::on_addSongButton_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Выберите аудиофайлы",
                                                      QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
                                                      "Аудиофайлы (*.mp3 *.wav *.flac);;Все файлы (*)");
    if (files.isEmpty()) {
        return;
    }

    for (const QString &filePath : files) {
        QFileInfo fileInfo(filePath);
        QString title = fileInfo.baseName();
        // Метаданные (artist, album, durationMs) можно получить позже, когда MusicPlayer начнет воспроизведение
        // Для простоты пока сохраняем базовое имя
        int songId = dbManager->addSong(filePath, title, "", "", 0); // Пустые artist, album, durationMs
        if (songId != -1) {
            QStandardItem *item = new QStandardItem(title);
            item->setData(songId, Qt::UserRole + 1);
            item->setData(filePath, Qt::UserRole + 2);
            songListModel->appendRow(item);
        }
    }
    ui->playButton->setEnabled(songListModel->rowCount() > 0);
    ui->deleteSongButton->setEnabled(songListModel->rowCount() > 0);
}

void MainWindow::on_createPlaylistButton_clicked()
{
    bool ok;
    QString playlistName = QInputDialog::getText(this, "Создать плейлист",
                                                 "Название плейлиста:", QLineEdit::Normal,
                                                 "", &ok);
    if (ok && !playlistName.isEmpty()) {
        int playlistId = dbManager->createPlaylist(playlistName);
        if (playlistId != -1) {
            QStandardItem *item = new QStandardItem(playlistName);
            item->setData(playlistId, Qt::UserRole + 1);
            playlistListModel->appendRow(item);
            QMessageBox::information(this, "Плейлист создан", "Плейлист '" + playlistName + "' успешно создан.");
        } else {
            // Проверить, если это ошибка уникальности имени
            QMessageBox::warning(this, "Ошибка", "Плейлист с таким названием уже существует или произошла другая ошибка.");
        }
    }
}

void MainWindow::on_deleteSongButton_clicked()
{
    QModelIndex currentIndex = ui->songListView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Удаление песни", "Пожалуйста, выберите песню для удаления.");
        return;
    }

    int songId = currentIndex.data(Qt::UserRole + 1).toInt();
    QString songTitle = currentIndex.data(Qt::DisplayRole).toString();
    QString songFilePath = currentIndex.data(Qt::UserRole + 2).toString();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Удалить песню",
                                  "Вы уверены, что хотите удалить '" + songTitle + "' из библиотеки?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) {
        return;
    }

    if (dbManager->deleteSong(songId)) {
        songListModel->removeRow(currentIndex.row());
        QMessageBox::information(this, "Удаление песни", "Песня '" + songTitle + "' успешно удалена.");

        // FIX: Use the public currentSource() method from MusicPlayer
        if (musicPlayer->playbackState() != QMediaPlayer::StoppedState &&
            musicPlayer->duration() > 0 &&
            musicPlayer->currentSource().toLocalFile() == songFilePath) { // Now this is correct!
            on_stopButton_clicked();
        }
    } else {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось удалить песню из базы данных.");
    }

    ui->deleteSongButton->setEnabled(songListModel->rowCount() > 0);
    ui->playButton->setEnabled(songListModel->rowCount() > 0);
}

// --- Слоты от MusicPlayer ---
void MainWindow::handlePlayerPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    updateUIForPlaybackState(state);
}

void MainWindow::handlePlayerPositionChanged(qint64 position)
{
    ui->progressBar->setValue(position);
    ui->currentTimeLabel->setText(formatTime(position));
}

void MainWindow::handlePlayerDurationChanged(qint64 duration)
{
    ui->progressBar->setMaximum(duration);
    ui->totalTimeLabel->setText(formatTime(duration));
    if (duration > 0) {
        ui->progressBar->setEnabled(true);
    }
}

void MainWindow::handlePlayerMetaDataChanged(const QString& title, const QString& artist, const QImage& albumArt)
{
    updateCurrentTrackInfo(title, artist);
    updateAlbumArt(albumArt);
}

void MainWindow::handlePlayerError(const QString& errorMessage)
{
    QMessageBox::critical(this, "Ошибка плеера", "Произошла ошибка воспроизведения: " + errorMessage);
    on_stopButton_clicked(); // Остановить воспроизведение при ошибке
}


// --- Слоты для выбора песен/плейлистов ---
void MainWindow::on_songListView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QString filePath = index.data(Qt::UserRole + 2).toString();
    if (!filePath.isEmpty()) {
        musicPlayer->setSource(filePath);
        musicPlayer->play();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось получить путь к файлу.");
    }
}

void MainWindow::on_playlistListView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int playlistId = index.data(Qt::UserRole + 1).toInt();
    QString playlistName = index.data(Qt::DisplayRole).toString();
    QMessageBox::information(this, "Плейлист", "Выбран плейлист: " + playlistName + " (ID: " + QString::number(playlistId) + ")");
    // Здесь можно реализовать логику загрузки песен из выбранного плейлиста и отображения их
}

// --- Вспомогательные методы ---
void MainWindow::updateUIForPlaybackState(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        ui->playButton->setEnabled(false);
        ui->pauseButton->setEnabled(true);
        ui->stopButton->setEnabled(true);
        break;
    case QMediaPlayer::PausedState:
        ui->playButton->setEnabled(true);
        ui->pauseButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
        break;
    case QMediaPlayer::StoppedState:
        ui->playButton->setEnabled(songListModel->rowCount() > 0); // Включаем кнопку Play, если есть песни
        ui->pauseButton->setEnabled(false);
        ui->stopButton->setEnabled(false);
        ui->progressBar->setEnabled(false);
        break;
    default:
        break;
    }
    ui->nextButton->setEnabled(songListModel->rowCount() > 1); // Предполагаем, что кнопки Next/Prev активны, если есть более 1 песни
    ui->previousButton->setEnabled(songListModel->rowCount() > 1);
}

QString MainWindow::formatTime(qint64 milliseconds)
{
    qint64 totalSeconds = milliseconds / 1000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::updateCurrentTrackInfo(const QString &title, const QString &artist)
{
    if (artist.isEmpty()) {
        ui->currentTrackLabel->setText(title);
    } else {
        ui->currentTrackLabel->setText(title + " - " + artist);
    }
}

void MainWindow::updateAlbumArt(const QImage &image)
{
    if (image.isNull()) {
        ui->albumArtLabel->setText("No Album Art");
        ui->albumArtLabel->setPixmap(QPixmap());
    } else {
        QPixmap pixmap = QPixmap::fromImage(image);
        ui->albumArtLabel->setPixmap(pixmap.scaled(ui->albumArtLabel->size(),
                                                   Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation));
        ui->albumArtLabel->setText("");
    }
}

void MainWindow::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        if (isRepeatEnabled) {
            // Если повтор включен, просто перезапускаем текущий трек с начала
            musicPlayer->setPosition(0);
            musicPlayer->play();
            qDebug() << "Repeating current track.";
        } else {
            // Если повтор выключен, переходим к следующему треку (или останавливаемся)
            on_nextButton_clicked();
        }
    }
}

void MainWindow::on_repeatButton_clicked()
{
    isRepeatEnabled = true;
    qDebug() << "Repeat is now:" << (true ? "ON" : "OFF");
}

