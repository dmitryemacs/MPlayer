#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTime>

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
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status); // Новый слот

    // Слоты для прогресс-бара и громкости
    void on_progressBar_sliderMoved(int position);
    void on_volumeSlider_valueChanged(int value);

    // Слоты для добавления песен и плейлистов
    void on_addSongButton_clicked();
    void on_createPlaylistButton_clicked();
    void on_deleteSongButton_clicked();

    // Слоты от MusicPlayer
    void handlePlayerPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void handlePlayerPositionChanged(qint64 position);
    void handlePlayerDurationChanged(qint64 duration);
    void handlePlayerMetaDataChanged(const QString& title, const QString& artist, const QImage& albumArt);
    void handlePlayerError(const QString& errorMessage);

    // Слоты для выбора песен/плейлистов
    void on_songListView_doubleClicked(const QModelIndex &index);
    void on_playlistListView_doubleClicked(const QModelIndex &index);


    void on_repeatButton_clicked();

private:
    Ui::MainWindow *ui;
    MusicPlayer *musicPlayer; // Теперь инстанс MusicPlayer
    DatabaseManager *dbManager; // Теперь инстанс DatabaseManager

    QStandardItemModel *songListModel;
    QStandardItemModel *playlistListModel;

    bool isRepeatEnabled = false;

    void initializeUIState();
    void loadDataIntoModels();
    void updateUIForPlaybackState(QMediaPlayer::PlaybackState state);
    void updateCurrentTrackInfo(const QString &title, const QString &artist);
    void updateAlbumArt(const QImage &image);
    QString formatTime(qint64 milliseconds);
};
#endif // MAINWINDOW_H
