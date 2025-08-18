#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentViewingPlaylistId(-1)
    , m_currentSongIndex(-1) // Инициализация нового члена
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
    dbManager->seedDatabase();

    // Инициализация моделей для QListView
    songListModel = new QStandardItemModel(this);
    ui->songListView->setModel(songListModel);

    playlistListModel = new QStandardItemModel(this);
    ui->playlistListView->setModel(playlistListModel);

    // Загрузка всех плейлистов при запуске
    QList<PlaylistInfo> playlists = dbManager->loadPlaylists();
    for (const PlaylistInfo& playlist : playlists) {
        QStandardItem *item = new QStandardItem(playlist.name);
        item->setData(playlist.id, Qt::UserRole + 1);  // Playlist ID
        playlistListModel->appendRow(item);
    }

    // Загрузка всех песен в songListModel и m_playbackQueue при запуске
    loadAllSongs();

    // Начальное состояние UI
    initializeUIState();

    // Соединение сигналов от MusicPlayer со слотами MainWindow
    connect(musicPlayer, &MusicPlayer::playbackStateChanged, this, &MainWindow::handlePlayerPlaybackStateChanged);
    connect(musicPlayer, &MusicPlayer::positionChanged, this, &MainWindow::handlePlayerPositionChanged);
    connect(musicPlayer, &MusicPlayer::durationChanged, this, &MainWindow::handlePlayerDurationChanged);
    connect(musicPlayer, &MusicPlayer::metaDataChanged, this, &MainWindow::handlePlayerMetaDataChanged);
    connect(musicPlayer, &MusicPlayer::errorOccurred, this, &MainWindow::handlePlayerError);
    connect(musicPlayer, &MusicPlayer::mediaStatusChanged, this, &MainWindow::handleMediaStatusChanged);

    // НОВЫЕ СОЕДИНЕНИЯ для контекстного меню и смены вкладок
    connect(ui->songListView, &QListView::customContextMenuRequested, this, &MainWindow::on_songListView_customContextMenuRequested);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidget_currentChanged);

    // Установка громкости по умолчанию
    musicPlayer->setVolume(ui->volumeSlider->value());

    // Если есть песни, выбираем первую
    if (!m_playbackQueue.isEmpty()) {
        ui->songListView->setCurrentIndex(songListModel->index(0, 0));
        m_currentSongIndex = 0;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete dbManager;
}

void MainWindow::initializeUIState()
{
    // Кнопки управления плеером
    bool hasSongs = !m_playbackQueue.isEmpty();
    ui->playButton->setEnabled(hasSongs);
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->nextButton->setEnabled(hasSongs && m_playbackQueue.size() > 1);
    ui->previousButton->setEnabled(hasSongs && m_playbackQueue.size() > 1);

    ui->progressBar->setEnabled(false);
    ui->deleteSongButton->setEnabled(hasSongs);
    ui->albumArtLabel->setText("No Album Art");
    ui->currentTrackLabel->setText("Нет трека");
    ui->currentTimeLabel->setText(formatTime(0));
    ui->totalTimeLabel->setText(formatTime(0));
    ui->progressBar->setValue(0);

    // Устанавливаем начальное состояние кнопки повтора
    ui->repeatButton->setChecked(isRepeatEnabled);

    // Устанавливаем начальный заголовок списка песен
    ui->currentSongListViewTitleLabel->setText("Библиотека песен");
}

// НОВАЯ ФУНКЦИЯ: Загружает все песни в songListModel и m_playbackQueue
void MainWindow::loadAllSongs()
{
    songListModel->clear();
    m_playbackQueue.clear(); // Очищаем очередь воспроизведения
    QList<SongInfo> songs = dbManager->loadSongs();
    for (const SongInfo& song : songs) {
        QString displayText; // ИЗМЕНЕНО
        if (!song.artist.isEmpty()) { // ИЗМЕНЕНО: Сначала исполнитель
            displayText = song.artist + " - " + song.title;
        } else {
            displayText = song.title; // Если исполнителя нет, просто название
        }
        QStandardItem *item = new QStandardItem(displayText);
        item->setData(song.id, Qt::UserRole + 1);      // Song ID
        item->setData(song.filePath, Qt::UserRole + 2); // File Path
        songListModel->appendRow(item);
        m_playbackQueue.append(song); // Добавляем в очередь воспроизведения
    }
    ui->currentSongListViewTitleLabel->setText("Библиотека песен");
    m_currentViewingPlaylistId = -1; // Сбрасываем ID просматриваемого плейлиста

    // Обновляем состояние кнопок после загрузки
    initializeUIState();
    if (!m_playbackQueue.isEmpty()) {
        m_currentSongIndex = 0; // Устанавливаем на первую песню
        ui->songListView->setCurrentIndex(songListModel->index(0, 0));
    } else {
        m_currentSongIndex = -1;
    }
}

// НОВАЯ ФУНКЦИЯ: Загружает песни для конкретного плейлиста в songListModel и m_playbackQueue
void MainWindow::loadSongsForPlaylist(int playlistId, const QString& playlistName)
{
    songListModel->clear();
    m_playbackQueue.clear(); // Очищаем очередь воспроизведения
    QList<SongInfo> songs = dbManager->getSongsInPlaylist(playlistId);
    for (const SongInfo& song : songs) {
        QString displayText; // ИЗМЕНЕНО
        if (!song.artist.isEmpty()) { // ИЗМЕНЕНО: Сначала исполнитель
            displayText = song.artist + " - " + song.title;
        } else {
            displayText = song.title; // Если исполнителя нет, просто название
        }
        QStandardItem *item = new QStandardItem(displayText);
        item->setData(song.id, Qt::UserRole + 1);      // Song ID
        item->setData(song.filePath, Qt::UserRole + 2); // File Path
        songListModel->appendRow(item);
        m_playbackQueue.append(song); // Добавляем в очередь воспроизведения
    }
    ui->currentSongListViewTitleLabel->setText("Плейлист: " + playlistName);
    m_currentViewingPlaylistId = playlistId; // Устанавливаем ID просматриваемого плейлиста

    // Обновляем состояние кнопок после загрузки
    initializeUIState();
    if (!m_playbackQueue.isEmpty()) {
        m_currentSongIndex = 0; // Устанавливаем на первую песню
        ui->songListView->setCurrentIndex(songListModel->index(0, 0));
    } else {
        m_currentSongIndex = -1;
    }
}

// НОВАЯ ФУНКЦИЯ: Воспроизводит песню по индексу из m_playbackQueue
void MainWindow::playSongAtIndex(int index)
{
    if (index < 0 || index >= m_playbackQueue.size()) {
        qDebug() << "Попытка воспроизвести песню по неверному индексу:" << index;
        musicPlayer->stop();
        m_currentSongIndex = -1; // Сбрасываем индекс, так как песня не найдена
        updateUIForPlaybackState(QMediaPlayer::StoppedState);
        return;
    }

    m_currentSongIndex = index;
    SongInfo song = m_playbackQueue.at(m_currentSongIndex);
    musicPlayer->setSource(song.filePath);
    musicPlayer->play();

    // Выделяем текущую песню в списке
    ui->songListView->setCurrentIndex(songListModel->index(m_currentSongIndex, 0));
}


// --- Слоты для кнопок управления плеером ---
void MainWindow::on_playButton_clicked()
{
    if (musicPlayer->playbackState() == QMediaPlayer::PausedState ||
        musicPlayer->playbackState() == QMediaPlayer::StoppedState) {
        if (m_playbackQueue.isEmpty()) {
            QMessageBox::information(this, "Нет песен", "Добавьте песни в библиотеку для воспроизведения.");
            return;
        }
        if (m_currentSongIndex == -1) { // Если нет выбранной песни, играем первую
            playSongAtIndex(0);
        } else {
            musicPlayer->play();
        }
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
    // m_currentSongIndex = -1; // Можно сбросить индекс, если хотим, чтобы Stop полностью "сбрасывал" воспроизведение
}

void MainWindow::on_nextButton_clicked()
{
    if (m_playbackQueue.isEmpty()) return;

    int nextIndex = m_currentSongIndex + 1;
    if (nextIndex >= m_playbackQueue.size()) {
        nextIndex = 0; // Переход к началу списка
    }
    playSongAtIndex(nextIndex);
}

void MainWindow::on_previousButton_clicked()
{
    if (m_playbackQueue.isEmpty()) return;

    int prevIndex = m_currentSongIndex - 1;
    if (prevIndex < 0) {
        prevIndex = m_playbackQueue.size() - 1; // Переход к концу списка
    }
    playSongAtIndex(prevIndex);
}

// Реализация слота для кнопки повтора
void MainWindow::on_repeatButton_toggled(bool checked)
{
    isRepeatEnabled = checked;
    qDebug() << "Repeat is now:" << (isRepeatEnabled ? "ON" : "OFF");
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
        int songId = dbManager->addSong(filePath, title, "", "", 0);
        if (songId != -1) {
            // После добавления песни в БД, обновляем текущий список, если это "Библиотека песен"
            if (m_currentViewingPlaylistId == -1) {
                // Чтобы избежать дублирования кода, просто перезагружаем все песни
                // Это также обновит m_playbackQueue
                loadAllSongs();
            }
            // Если просматривается плейлист, песня добавится в БД, но не в текущий список песен
            // Пользователь должен будет переключиться на "Библиотека песен", чтобы увидеть ее
        }
    }
    // Обновляем состояние кнопок после добавления
    initializeUIState();
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
        QMessageBox::information(this, "Удаление песни", "Песня '" + songTitle + "' успешно удалена.");

        // Если удаляемая песня была текущей воспроизводимой
        if (musicPlayer->playbackState() != QMediaPlayer::StoppedState &&
            musicPlayer->currentSource().toLocalFile() == songFilePath) {
            on_stopButton_clicked();
        }

        // Перезагружаем текущий список, чтобы обновить songListModel и m_playbackQueue
        if (m_currentViewingPlaylistId == -1) {
            loadAllSongs();
        } else {
            // Если удаляем из плейлиста, нужно обновить список песен этого плейлиста
            // Сначала получаем имя плейлиста, чтобы передать его в loadSongsForPlaylist
            QString playlistName = "";
            for (int row = 0; row < playlistListModel->rowCount(); ++row) {
                QStandardItem *item = playlistListModel->item(row);
                if (item->data(Qt::UserRole + 1).toInt() == m_currentViewingPlaylistId) {
                    playlistName = item->text();
                    break;
                }
            }
            loadSongsForPlaylist(m_currentViewingPlaylistId, playlistName);
        }
    } else {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось удалить песню из базы данных.");
    }
    // Обновляем состояние кнопок после удаления
    initializeUIState();
}

void MainWindow::on_deletePlaylistButton_clicked()
{
    QModelIndex currentIndex = ui->playlistListView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "Удаление плейлиста", "Пожалуйста, выберите плейлист для удаления.");
        return;
    }

    int playlistId = currentIndex.data(Qt::UserRole + 1).toInt();
    QString playlistName = currentIndex.data(Qt::DisplayRole).toString();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Удалить плейлист",
                                  "Вы уверены, что хотите удалить плейлист '" + playlistName + "'?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) {
        return;
    }

    if (dbManager->deletePlaylist(playlistId)) {
        playlistListModel->removeRow(currentIndex.row());
        QMessageBox::information(this, "Удаление плейлиста", "Плейлист '" + playlistName + "' успешно удален.");

        // Если удаленный плейлист был текущим просматриваемым, переключиться на "Все песни"
        if (m_currentViewingPlaylistId == playlistId) {
            loadAllSongs();
        }
    } else {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось удалить плейлист из базы данных.");
    }
    // Обновляем состояние кнопок после удаления
    initializeUIState();
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

void MainWindow::handlePlayerMetaDataChanged(const QString& title, const QString& artist, const QString& album, const QImage& albumArt)
{
    updateCurrentTrackInfo(title, artist);
    updateAlbumArt(albumArt);

    QString currentFilePath = musicPlayer->currentSource().toLocalFile();
    qint64 durationMs = musicPlayer->duration();

    int songId = dbManager->addSong(currentFilePath, title, artist, album, durationMs);

    if (songId != -1) {
        qDebug() << "Метаданные песни (ID:" << songId << ") обновлены в БД: " << title << " - " << artist << " - " << album << " (" << durationMs << "ms)";

        // Обновляем отображение в QListView, если метаданные изменились
        for (int row = 0; row < songListModel->rowCount(); ++row) {
            QStandardItem *item = songListModel->item(row);
            if (item->data(Qt::UserRole + 2).toString() == currentFilePath) {
                QString newDisplayText; // ИЗМЕНЕНО
                if (!artist.isEmpty()) { // ИЗМЕНЕНО: Сначала исполнитель
                    newDisplayText = artist + " - " + title;
                } else {
                    newDisplayText = title; // Если исполнителя нет, просто название
                }
                if (item->text() != newDisplayText) {
                    item->setText(newDisplayText);
                }
                break;
            }
        }
    }
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

    // Находим индекс в m_playbackQueue
    m_currentSongIndex = index.row();
    playSongAtIndex(m_currentSongIndex);
}

void MainWindow::on_playlistListView_doubleIndexClicked(const QModelIndex &index) // Переименованный слот
{
    if (!index.isValid()) return;
    int playlistId = index.data(Qt::UserRole + 1).toInt();
    QString playlistName = index.data(Qt::DisplayRole).toString();

    loadSongsForPlaylist(playlistId, playlistName); // Загружаем песни выбранного плейлиста
    ui->tabWidget->setCurrentIndex(0); // Переключаемся на вкладку "Библиотека песен"
}

// НОВЫЙ СЛОТ: Обработка запроса контекстного меню для songListView
void MainWindow::on_songListView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->songListView->indexAt(pos);
    if (!index.isValid()) {
        return; // Если клик был не по элементу, не показываем меню
    }

    int songId = index.data(Qt::UserRole + 1).toInt();
    QString songTitle = index.data(Qt::DisplayRole).toString();

    QMenu contextMenu(this);
    QMenu *addToPlaylistMenu = contextMenu.addMenu("Добавить в плейлист");

    QList<PlaylistInfo> playlists = dbManager->loadPlaylists();

    if (playlists.isEmpty()) {
        addToPlaylistMenu->addAction("Нет плейлистов")->setEnabled(false);
    } else {
        for (const PlaylistInfo& playlist : playlists) {
            QAction *action = addToPlaylistMenu->addAction(playlist.name);
            // Используем лямбда-функцию для передачи songId и playlistId в слот
            connect(action, &QAction::triggered, this, [this, songId, playlistId = playlist.id]() {
                addSongToSpecificPlaylist(songId, playlistId);
            });
        }
    }

    contextMenu.exec(ui->songListView->mapToGlobal(pos));
}

// НОВЫЙ СЛОТ: Добавление песни в выбранный плейлист
void MainWindow::addSongToSpecificPlaylist(int songId, int playlistId)
{
    // Для song_order можно использовать 0 или найти максимальный существующий order в плейлисте
    // Для простоты пока используем 0, т.к. ON CONFLICT предотвратит дублирование
    if (dbManager->addSongToPlaylist(playlistId, songId, 0)) {
        QMessageBox::information(this, "Добавление в плейлист", "Песня успешно добавлена в плейлист.");

        // Если текущий просматриваемый список песен - это тот же плейлист,
        // то обновляем его, чтобы новая песня появилась сразу
        if (m_currentViewingPlaylistId == playlistId) {
            // Находим имя плейлиста по ID для обновления заголовка
            QString playlistName = "";
            QList<PlaylistInfo> playlists = dbManager->loadPlaylists();
            for (const PlaylistInfo& p : playlists) {
                if (p.id == playlistId) {
                    playlistName = p.name;
                    break;
                }
            }
            loadSongsForPlaylist(playlistId, playlistName);
        }
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось добавить песню в плейлист. Возможно, она уже там.");
    }
}

// НОВЫЙ СЛОТ: Обработка смены вкладок
void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 0) { // Если выбрана вкладка "Библиотека песен"
        loadAllSongs(); // Загружаем все песни
    }
    // Если выбрана вкладка "Плейлисты" (индекс 1), ничего не делаем,
    // так как playlistListModel уже загружен в конструкторе,
    // а песни плейлиста загружаются по двойному клику.
}


// --- Вспомогательные методы ---
void MainWindow::updateUIForPlaybackState(QMediaPlayer::PlaybackState state)
{
    bool hasSongs = !m_playbackQueue.isEmpty();
    bool hasMultipleSongs = m_playbackQueue.size() > 1;

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
        ui->playButton->setEnabled(hasSongs); // Включаем кнопку Play, если есть песни
        ui->pauseButton->setEnabled(false);
        ui->stopButton->setEnabled(false);
        ui->progressBar->setEnabled(false);
        break;
    default:
        break;
    }
    ui->nextButton->setEnabled(hasMultipleSongs);
    ui->previousButton->setEnabled(hasMultipleSongs);
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
        ui->currentTrackLabel->setText(artist + " - " + title);
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
            musicPlayer->setPosition(0);
            musicPlayer->play();
            qDebug() << "Repeating current track.";
        } else {
            on_nextButton_clicked(); // Автоматический переход к следующему треку
        }
    }
}
