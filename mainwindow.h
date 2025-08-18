#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTime>
#include <QMenu>

// Включаем новые заголовочные файлы
#include "database_manager.h"
#include "music_player.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Слоты для кнопок управления плеером
    void on_playButton_clicked();
    void on_pauseButton_clicked();
    void on_stopButton_clicked();
    void on_nextButton_clicked();
    void on_previousButton_clicked();

    void on_repeatButton_toggled(bool checked);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);

    // Слоты для прогресс-бара и громкости
    void on_progressBar_sliderMoved(int position);
    void on_volumeSlider_valueChanged(int value);

    // Слоты для добавления песен и плейлистов
    void on_addSongButton_clicked();
    void on_createPlaylistButton_clicked();
    void on_deleteSongButton_clicked();
    void on_deletePlaylistButton_clicked();

    // Слоты от MusicPlayer
    void handlePlayerPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void handlePlayerPositionChanged(qint64 position);
    void handlePlayerDurationChanged(qint64 duration);
    void handlePlayerMetaDataChanged(const QString& title, const QString& artist, const QString& album, const QImage& albumArt);
    void handlePlayerError(const QString& errorMessage);

    // Слоты для выбора песен/плейлистов
    void on_songListView_doubleClicked(const QModelIndex &index);
    void on_playlistListView_doubleIndexClicked(const QModelIndex &index); // Исправлено название слота для ясности

    // Слоты для контекстного меню и управления списками
    void on_songListView_customContextMenuRequested(const QPoint &pos);
    void addSongToSpecificPlaylist(int songId, int playlistId);
    void on_tabWidget_currentChanged(int index);

private:
    Ui::MainWindow *ui;
    MusicPlayer *musicPlayer;
    DatabaseManager *dbManager;

    QStandardItemModel *songListModel;
    QStandardItemModel *playlistListModel;

    bool isRepeatEnabled = false;

    int m_currentViewingPlaylistId; // -1, если показываются все песни; ID плейлиста, если показываются песни плейлиста

    // НОВЫЕ ПЕРЕМЕННЫЕ ДЛЯ ВОСПРОИЗВЕДЕНИЯ
    QList<SongInfo> m_playbackQueue; // Текущий список песен для воспроизведения
    int m_currentSongIndex;         // Индекс текущей песни в m_playbackQueue

    void initializeUIState();
    void loadAllSongs();
    void loadSongsForPlaylist(int playlistId, const QString& playlistName);

    // НОВАЯ ФУНКЦИЯ: Воспроизводит песню по индексу из m_playbackQueue
    void playSongAtIndex(int index);

    void updateUIForPlaybackState(QMediaPlayer::PlaybackState state);
    void updateCurrentTrackInfo(const QString &title, const QString &artist);
    void updateAlbumArt(const QImage &image);
    QString formatTime(qint64 milliseconds);
};
#endif // MAINWINDOW_H
