#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QList>
#include <QAudio>
#include <QAudioDevice>
#include <QCamera>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QVideoWidget>
#include <QMediaCaptureSession>
#include <QMainWindow>
#include <QStandardPaths>
#include <QString>
#include <QAudio>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QAudioInput>
#include <QStandardPaths>
#include <QUrl>
#include <QMediaPlayer>
#include <QLabel>
#include <QImageCapture>
#include <QVBoxLayout>
#include <QComboBox>

namespace mApp {
enum AppState{
    STATE_INACTIVE=0,
    STATE_CAMERA_OPEN,
    STATE_RECORDING_VOICE,
    STATE_RECORDING_VIDEO,
    STATE_AUDIO_PLAYING,
    STATE_AUDIO_PAUSED,
    STATE_VIDEO_PLAYING,
    STATE_VIDEO_PAUSED
};
enum RecordingType {
    RECORD_TYPE_NONE=0,
    RECORD_TYPE_CAMERA,
    RECORD_TYPE_VIDEO,
    RECORD_TYPE_AUDIO,
};
enum RecordingState {
    RECORDING_STOPPED=0,
    RECORDING_PAUSED,
    RECORDING_ACTIVE
};

enum MediaPlayerState {
    MEDIA_NONE=0,
    MEDIA_PLAY,
    MEDIA_PAUSE
};

enum TabType {
    TAB_MEDIA_RECORDER=0,
    TAB_MEDIA_PLAYER
};
}


class MediaPlayer;
class ThemeHandler;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    Ui::MainWindow * getMainUi() const {
        return ui;
    }
protected:
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
private slots:


private:
    void logAboutAvailableMediaDevices();
    // -------------------------------------------------------------
    void handleMediaCaptureEvent();
    void handleRecordingTypeChange();
    void handleSaveMediaButton();
    void handleTabChanged(int);
    // -------------------------------------------------------------
    void setToImageCaptureMode();
    void setToVideoCaptureMode();
    void setToAudioCaptureMode();
    // -------------------------------------------------------------
    void setRecState(mApp::RecordingState); // for all states
    void setRecStateRecording();
    void setRecStatePaused();
    void setRecStateStopped();
    // ---------------------- Image Capture ------------------------
    void showCamera();
    void closeCamera();
    void captureImage();
    void showImagePreview(int id, const QImage &preview);
    void hideImagePreview();
    void saveImageCaptured();
    // ---------------------- ------------- ------------------------
    // ---------------------- Video Capture ------------------------
    void startVideoRecording();
    void pauseVideoRecording();
    void resumeVideoRecording();
    void saveVideoRecording();
    // ---------------------- ------------- ------------------------
    // ---------------------- Audio Capture ------------------------
    void startAudioRecording();
    void pauseAudioRecording();
    void resumeAudioRecording();
    void saveAudioRecording();
    // ---------------------- ------------- ------------------------
    void handleMediaPlayerToggleButton();

    void setWindowShown();

    void connectSlots();

    void cleanPlayerWidget(QVBoxLayout*);


    Ui::MainWindow *ui;

    // device
    QList<QCameraDevice> m_cameras;
    QList<QAudioDevice>  m_microphones;

    QCamera* m_camera = nullptr;
    QCameraDevice* m_cameraDevice = nullptr;

    QLabel* m_imagePreviewLabel = nullptr;
    QImage m_capturedImage;  // Stores the captured image for saving later

    // action on device. capture/record
    QMediaCaptureSession* m_captureSession = nullptr;
    QImageCapture *m_imageCapture = nullptr;

    QMediaRecorder
        *m_audioRecorder = nullptr,
        *m_videoRecorder = nullptr;

    QAudioInput* m_audioInput = nullptr;
    QVideoWidget* m_videoWidget = nullptr;

    // Camera; Video; Audio;
    mApp::RecordingType m_recorderButtonType = mApp::RECORD_TYPE_NONE;
    // Only for audio/video: nothing, recording, paused.
    mApp::RecordingState m_multimediaRecordingState = mApp::RECORDING_STOPPED;

    MediaPlayer* m_mediaPlayerHandler = nullptr;
    ThemeHandler* m_themeHandler = nullptr;
};
#endif // MAINWINDOW_H
