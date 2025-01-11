#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QObject>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QVBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPushButton>


namespace mApp{
enum RenderType {
    Rendering_None=0,
    Rendering_Image,
    Rendering_Audio,
    Rendering_Video
};
}

class MainWindow;
namespace Ui {
class MainWindow;
}

class ImageCropper;

class MediaPlayer : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlayer(MainWindow* mainWindow, QObject* parent = nullptr);
    ~MediaPlayer();


    void buttonHandler(QKeyEvent* event);

    void setMediaPlayerNoMediaState() {
        setMediaPlayerNoMediaStateInternal();
    }
    void goFullScreenVideo(QKeyEvent*);
    // Utility function for formatting time
    QString formatTime(qint64 ms) const;
signals:

protected:

private:
    void playMedia(const QString& filePath);
    void pauseMediaPlayer();
    void resumeMediaPlayer();
    void stopMediaPlayer();
    void handleMute();

    void showWidget(QWidget* widget);
    void showNoneWidget();
    void showImageWidget();
    void showAudioWidget();
    void showVideoWidget();

    void loadMedia();

    void loadImage(const QString &filePath);

    //------------------------- Media Player State------------------------------
    void setMediaPlayerNoMediaStateInternal();
    void setMediaPlayerLoadedImageState();
    void setMediaPlayerPlayingState();
    void setMediaPlayerPausedState();

    void handleNextPressed();
    void handlePrevPressed();
    void handleRestartPressed();
    void handleButtonCodec();

    void handleMediaPlayerToggleButton();
    void handleTabChanged(int);
    void handlePlaybackSlider(int);
    void handleVolumeSlider(int);

    void handleNextAudioTrack();

    void setFullScreen();
    void unsetFullScreen();

    void hideCodecButton();
    void showCodecButton();


    void connectSlots();

    QLayout* m_vLayoutMediaPlayer = nullptr;

    ImageCropper* m_imageLabel = nullptr;
    // QLabel* m_imageLabel = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QVideoWidget* m_videoWidget = nullptr;
    QMediaPlayer* m_mediaPlayer = nullptr;


    MainWindow* m_mainWindow;          // Store a pointer to MainWindow
    Ui::MainWindow * mainUi = nullptr;

    mApp::RenderType m_renderingType = mApp::Rendering_None;

    bool isFullscreen = false;
};

#endif // MEDIAPLAYER_H
