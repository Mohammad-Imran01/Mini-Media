#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "src/theme/themehandler.h"
#include "mediaplayer.h"

#include <QAudioOutput>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMediaFormat>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_captureSession (new QMediaCaptureSession(this))
    , m_audioRecorder (new QMediaRecorder(this))
    , m_videoRecorder (new QMediaRecorder(this))
    , m_audioInput (new QAudioInput(this))
    , m_videoWidget (new QVideoWidget(this))
    , m_imageCapture (new QImageCapture(this))
    , m_imagePreviewLabel (new QLabel(this))
{
    ui->setupUi(this);

    m_cameras     = QMediaDevices::videoInputs();
    m_microphones = QMediaDevices::audioInputs();

    setFocusPolicy(Qt::NoFocus);

    if(!m_cameras.isEmpty())
        m_camera = new QCamera(m_cameras.first(), this);
    else
        qWarning() << Q_FUNC_INFO << "Camera unavailable";

    ui->vLayoutForCamera->addWidget(m_videoWidget);

    m_captureSession->setImageCapture(m_imageCapture);
    m_captureSession->setCamera(m_camera);

    m_captureSession->setRecorder(m_videoRecorder);
    m_captureSession->setVideoOutput(m_videoWidget);


    m_captureSession->setAudioInput(m_audioInput);

    if(!m_microphones.isEmpty())
        m_audioInput->setDevice(m_microphones.first()); // Use the first microphone device
    else
        qWarning() << Q_FUNC_INFO << "Microphone Unavailable.";


    m_imagePreviewLabel->setStyleSheet(QString::fromUtf8("background-color: rgba(0, 0, 0, 150);"));
    m_imagePreviewLabel->setAlignment(Qt::AlignCenter);
    m_imagePreviewLabel->hide();

    logAboutAvailableMediaDevices();

    handleRecordingTypeChange();


    m_mediaPlayerHandler = new MediaPlayer(this, this);

    connectSlots();

    if(ui->mainTabWidget->count() > 0)
        ui->mainTabWidget->setCurrentIndex(0);


    setWindowShown();
    setToImageCaptureMode();

    m_themeHandler = new ThemeHandler(this, this);
}


void MainWindow::connectSlots()
{
    connect(m_imageCapture, &QImageCapture::imageCaptured, this, [](int id, const QImage &image) {
        Q_UNUSED (id)
        qDebug() << "Image captured:" << image.size();
    });

    connect(m_imageCapture, &QImageCapture::imageSaved, this, [](int id, const QString &filePath) {
        Q_UNUSED (id)
        qDebug() << "Image saved at:" << filePath;
    });

    connect(m_imageCapture, &QImageCapture::imageCaptured, this, &MainWindow::showImagePreview);

    connect(ui->pushButtonNextButtonRecorder, &QPushButton::clicked, this, &MainWindow::handleRecordingTypeChange);

    connect(ui->pushButtonCaptureMedia, &QPushButton::clicked, this, &MainWindow::handleMediaCaptureEvent);
    connect(ui->pushButtonCancelRec, &QPushButton::clicked, this, &MainWindow::hideImagePreview);
    connect(ui->pushButtonSaveMediaRec, &QPushButton::clicked, this, &MainWindow::handleSaveMediaButton);

    connect(m_audioRecorder, &QMediaRecorder::durationChanged, this, [this](qint64 duration) {
        if (m_audioRecorder->recorderState() != QMediaRecorder::PausedState) {
            ui->labelRecordingTimer->setText(m_mediaPlayerHandler->formatTime(duration));
        }
    });

    connect(m_videoRecorder, &QMediaRecorder::durationChanged, this, [this](qint64 duration) {
        if (m_videoRecorder->recorderState() != QMediaRecorder::PausedState) {
            ui->labelRecordingTimer->setText(m_mediaPlayerHandler->formatTime(duration));
        }
    });

    connect(ui->mainTabWidget, &QTabWidget::currentChanged, this, &MainWindow::handleTabChanged);    
}



/* when the capture button pressed: click image, record video, or audio ?? */
void MainWindow::handleMediaCaptureEvent()
{
    qDebug() << Q_FUNC_INFO << " ";
    switch (m_recorderButtonType) {
    case mApp::RECORD_TYPE_NONE: {
        qDebug() << Q_FUNC_INFO << " ";
        return;
    }
    case mApp::RECORD_TYPE_CAMERA: { // camera mode: button pressed
        captureImage();
        break;
    }
    case mApp::RECORD_TYPE_VIDEO: { // video mode: button pressed
        switch (m_multimediaRecordingState) {
        case mApp::RECORDING_STOPPED: {
            startVideoRecording();
            break;
        }// Just camera is open, no recording is happening: start video recording

        case mApp::RECORDING_PAUSED:{
            resumeVideoRecording();
            break;
        }// Video recording is paused: resume it.

        case mApp::RECORDING_ACTIVE:{
            pauseVideoRecording();
            break;
        }// Video recording is Active Pause it.

        default:
            break;
        }
        break;
    }
    case mApp::RECORD_TYPE_AUDIO: { // audio mode: button pressed
        switch (m_multimediaRecordingState) {
        case mApp::RECORDING_STOPPED: {
            startAudioRecording();
            break;
        }// Just camera is open, no recording is happening: start Audio recording

        case mApp::RECORDING_PAUSED:{
            qDebug() << "audio resume button";
            resumeAudioRecording();
            break;
        }// Audio recording is paused: resume it.

        case mApp::RECORDING_ACTIVE:{
            qDebug() << "audio pause button";
            pauseAudioRecording();
            break;
        }// Video recording is Active Pause it.

        default:
            break;
        }
        break;    }
    default: {
        qWarning() << Q_FUNC_INFO << "Something ain't right. Can't select next recorder button";
    }
    }
}
/* Next Media record: Media capture Mode Button changed.*/
void MainWindow::handleRecordingTypeChange() {
    qDebug() << Q_FUNC_INFO << " ";
    switch (m_recorderButtonType) {
    case mApp::RecordingType::RECORD_TYPE_NONE: {
        saveAudioRecording();
        setToImageCaptureMode();
        break;
    }
    case mApp::RecordingType::RECORD_TYPE_CAMERA: {
        setToVideoCaptureMode();
        break;
    }
    case mApp::RecordingType::RECORD_TYPE_VIDEO: {
        saveVideoRecording();
        setToAudioCaptureMode();
        break;
    }
    case mApp::RecordingType::RECORD_TYPE_AUDIO: {
        saveAudioRecording();
        setToImageCaptureMode();
        break;
    }
    default: {
        qWarning() << Q_FUNC_INFO << "Something ain't right. Can't select next recorder button";
    }
    }
    ui->pushButtonSaveMediaRec->setDisabled(true);
    ui->pushButtonCancelRec->setDisabled(true);
}
void MainWindow::handleSaveMediaButton()
{
    qDebug() << Q_FUNC_INFO << " ";
    switch (m_recorderButtonType) {
    case mApp::RecordingType::RECORD_TYPE_NONE: {
        return;
    }
    case mApp::RecordingType::RECORD_TYPE_CAMERA: {
        saveImageCaptured();
        break;
    }
    case mApp::RecordingType::RECORD_TYPE_VIDEO: {
        saveVideoRecording();
        break;
    }
    case mApp::RecordingType::RECORD_TYPE_AUDIO: {
        saveAudioRecording();
        break;
    }
    default: {
        qWarning() << Q_FUNC_INFO << "Something ain't right. Can't select next recorder button";
    }
    }
}



void MainWindow::handleTabChanged(int currentTab) {
    if(currentTab == static_cast<int>(mApp::TAB_MEDIA_PLAYER)) {
        switch (m_recorderButtonType) {
        case mApp::RECORD_TYPE_NONE: {
            qDebug() << Q_FUNC_INFO << "No recording or capture event is active.";
            break;
        }
        case mApp::RECORD_TYPE_CAMERA: {
            closeCamera();
            break;
        }
        case mApp::RECORD_TYPE_VIDEO: {
            saveVideoRecording();
            closeCamera();
            break;
        }
        case mApp::RECORD_TYPE_AUDIO: {
            saveAudioRecording();
            break;
        }
        default: break;
            qWarning() << Q_FUNC_INFO << "Unable to detect 'current capture' type.";
            break;
        }
    } else {
        switch (m_recorderButtonType) {
        case mApp::RECORD_TYPE_NONE: {
            qDebug() << Q_FUNC_INFO << "No recording or capture event is active.";
            break;
        }
        case mApp::RECORD_TYPE_CAMERA: {
            setToImageCaptureMode();
            break;
        }
        case mApp::RECORD_TYPE_VIDEO: {
            setToImageCaptureMode();
            setToVideoCaptureMode();
            break;
        }
        case mApp::RECORD_TYPE_AUDIO: {
            setToAudioCaptureMode();
            break;
        }
        default: break;
            qWarning() << Q_FUNC_INFO << "Unable to detect 'current capture' type.";
            break;
        }
    }
}

// ------------------- CAMERA ==============================
void MainWindow::showCamera()
{
    if (m_cameras.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "No camera device found.";
        return;
    }

    m_videoWidget->show();
    m_camera->start();

    qDebug() << "Camera started with device:" << m_cameras.first().description();
}
void MainWindow::closeCamera()
{
    m_videoWidget->hide();
    if (m_camera)
        m_camera->stop();

    qDebug() << Q_FUNC_INFO << "Camera closed.";
}
void MainWindow::captureImage()
{
    if (!m_camera) {
        qDebug() << Q_FUNC_INFO << "Camera not started.";
        return;
    }

    m_imageCapture->capture();
    qDebug() << Q_FUNC_INFO << "Captured an Image.";
}
void MainWindow::showImagePreview(int id, const QImage &preview)
{
    Q_UNUSED(id);

    m_capturedImage = QImage();

    if (!preview.isNull() && m_imagePreviewLabel) {
        m_capturedImage = preview;

        QPixmap pixmap = QPixmap::fromImage(preview);
        m_imagePreviewLabel->setPixmap(pixmap.scaled(
            m_videoWidget->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        m_videoWidget->hide();

        ui->vLayoutForCamera->removeWidget(m_videoWidget);
        ui->vLayoutForCamera->addWidget(m_imagePreviewLabel);

        ui->pushButtonCaptureMedia->setDisabled(true);

        m_imagePreviewLabel->show();

        ui->pushButtonCancelRec->setDisabled(false);
        ui->pushButtonNextButtonRecorder->setDisabled(true);
        ui->pushButtonSaveMediaRec->setDisabled(false);


        qDebug() << "Image preview displayed.";
    } else {
        qDebug() << "Failed to display image preview.";
    }
}
void MainWindow::hideImagePreview()
{
    if (m_imagePreviewLabel) {
        m_imagePreviewLabel->hide();

        ui->vLayoutForCamera->removeWidget(m_imagePreviewLabel);
        ui->vLayoutForCamera->addWidget(m_videoWidget);

        m_videoWidget->show();

        ui->pushButtonCaptureMedia->setDisabled(false);
        ui->pushButtonCancelRec->setDisabled(true);
        ui->pushButtonNextButtonRecorder->setDisabled(false);
        ui->pushButtonSaveMediaRec->setDisabled(true);


        qDebug() << Q_FUNC_INFO << "Image preview hidden.";
    }
}
void MainWindow::saveImageCaptured()
{
    if (m_capturedImage.isNull()) {
        qDebug() << "No image to save.";
        QMessageBox::warning(this, "Save Failed", "No image has been captured to save.");
        return;
    }


    // Ask the user for a file location to save the image
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Captured Image"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/captured_image.jpg",
        tr("Images (*.png *.jpg *.jpeg *.bmp)")
        );

    if (filePath.isEmpty()) {
        qDebug() << "Save operation cancelled.";
        return;
    }

    // Save the image to the selected location
    if (m_capturedImage.save(filePath)) {
        qDebug() << "Image successfully saved at:" << filePath;
        QMessageBox::information(this, "Image Saved", "Image has been saved successfully!");
    } else {
        qDebug() << "Failed to save image at:" << filePath;
        QMessageBox::warning(this, "Save Failed", "Failed to save the image.");
    }
    hideImagePreview();
    showCamera();
}
// -------------------------- -------------------------- ------------------------------
// ---------------------  VIDEO RECORDING =============================================
void MainWindow::startVideoRecording()
{
    if (!m_camera || !m_videoRecorder) {
        qDebug() << Q_FUNC_INFO << "Camera not started.";
        return;
    }

    if (m_videoRecorder->recorderState() == QMediaRecorder::RecordingState) {
        qWarning() << Q_FUNC_INFO << "Already recording.";
        return;
    }

    m_captureSession->setRecorder(m_videoRecorder);

    m_videoRecorder->record();

    if (m_videoRecorder->recorderState() == QMediaRecorder::RecordingState) {
        // qDebug() << Q_FUNC_INFO << "Recording started. Output file:" << filePath;
        setRecStateRecording();
    } else {
        qDebug() << Q_FUNC_INFO << "Failed to start recording.";
    }
}

void MainWindow::pauseVideoRecording()
{
    if(!m_videoRecorder) {
        qWarning() << Q_FUNC_INFO << "Uninitialized m_recorder";
        return;
    }

    if (m_videoRecorder->recorderState() != QMediaRecorder::RecordingState) {
        qDebug() << Q_FUNC_INFO << "Cannot pause. Recorder is not in a recording state.";
        return;
    }

    m_videoRecorder->pause();

    setRecStatePaused();

    qDebug() << Q_FUNC_INFO << "Recording paused.";
}
void MainWindow::resumeVideoRecording()
{
    if(!m_videoRecorder) {
        qWarning() << Q_FUNC_INFO << "Uninitialized m_recorder";
        return;
    }

    if (m_videoRecorder->recorderState() != QMediaRecorder::PausedState) {
        qWarning() << Q_FUNC_INFO << "Recorder is not in a paused state.";
        return;
    }

    m_videoRecorder->record();

    setRecStateRecording();

    qDebug() << Q_FUNC_INFO << "Recording resumed.";
}
void MainWindow::saveVideoRecording()
{
    if(!m_videoRecorder) {
        qWarning() << Q_FUNC_INFO << "Uninitialized m_recorder";
        return;
    }

    if (m_videoRecorder->recorderState() == QMediaRecorder::RecordingState ||
        m_videoRecorder->recorderState() == QMediaRecorder::PausedState)
    {
        m_videoRecorder->stop();
        setRecStateStopped();
    } else {
        qDebug() << Q_FUNC_INFO << "Cant save not recoding/paused state.";
    }

    qDebug() << Q_FUNC_INFO << "Recording saved." << m_audioRecorder->actualLocation();
}
// --------------------------------------- =============================================
// ----------------------------------- Audio capture -----------------------------------
void MainWindow::startAudioRecording()
{
    if (!m_audioRecorder) {
        qWarning() << Q_FUNC_INFO << "Audio recorder is uninitialized.";
        return;
    }

    if (m_microphones.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "No microphone devices available.";
        return;
    }

    if (m_audioRecorder->recorderState() == QMediaRecorder::RecordingState) {
        qWarning() << Q_FUNC_INFO << "Recording already in progress.";
        return;
    }

    m_captureSession->setRecorder(m_audioRecorder);

    // Start recording
    m_audioRecorder->record();

    if (m_audioRecorder->recorderState() == QMediaRecorder::RecordingState) {
        // qDebug() << Q_FUNC_INFO << "Recording started. Output file:" << filePath;
        setRecStateRecording(); // Update UI/state indicator
    } else {
        qWarning() << Q_FUNC_INFO << "Failed to start recording.";
    }
}

void MainWindow::pauseAudioRecording()
{
    if (!m_audioRecorder) {
        qWarning() << Q_FUNC_INFO << "Audio recorder is uninitialized.";
        return;
    }

    if (m_audioRecorder->recorderState() != QMediaRecorder::RecordingState) {
        qWarning() << Q_FUNC_INFO << "Cannot pause. Recorder is not in a recording state.";
        return;
    }

    m_audioRecorder->pause();
    setRecStatePaused(); // Update UI/state indicator
    qDebug() << Q_FUNC_INFO << "Recording paused.";
}
void MainWindow::resumeAudioRecording()
{
    if (!m_audioRecorder) {
        qWarning() << Q_FUNC_INFO << "Audio recorder is uninitialized.";
        return;
    }

    if (m_audioRecorder->recorderState() != QMediaRecorder::PausedState) {
        qWarning() << Q_FUNC_INFO << "Cannot resume. Recorder is not in a paused state.";
        return;
    }

    m_audioRecorder->record();
    setRecStateRecording(); // Update UI/state indicator
}
void MainWindow::saveAudioRecording()
{
    if (!m_audioRecorder) {
        qWarning() << Q_FUNC_INFO << "Audio recorder is uninitialized.";
        return;
    }

    if (m_audioRecorder->recorderState() == QMediaRecorder::RecordingState ||
        m_audioRecorder->recorderState() == QMediaRecorder::PausedState)
    {
        m_audioRecorder->stop();
        setRecStateStopped(); // Update UI/state indicator
    } else {
        qDebug() << Q_FUNC_INFO << "Cant save not recoding/paused state.";
    }

    qDebug() << Q_FUNC_INFO << "Recording saved." << m_audioRecorder->actualLocation();
}

void MainWindow::setWindowShown() {
    setWindowTitle(tr("Mini Media"));

    setWindowIcon(QIcon(":/miniMedia.ico"));

    ui->pushButtonCaptureMedia->setToolTip(tr("Capture media"));
    ui->pushButtonNextButtonRecorder->setToolTip(tr("Next button for media recorder"));
    ui->pushButtonSaveMediaRec->setToolTip(tr("Save recorded media"));
    ui->pushButtonCancelRec->setToolTip(tr("Cancel media recording"));

    ui->pushButtonMediaRestart->setToolTip(tr("Restart the media"));

    ui->pushButtonMediaPrev->setToolTip(tr("Jump back: left Arrow"));
    ui->pushButtonToggleMedia->setToolTip(tr("Play/Pause: space Bar"));
    ui->pushButtonMediaNext->setToolTip(tr("Jump forward: right Arrow"));
    ui->pushButtonLoadMedia->setToolTip(tr("Load media(Image/Audio/Video) file: O"));
    ui->pushButtonSound->setToolTip(tr("Mute/Unmute sound: M"));

    ui->sliderMediaPlayerVolume->setToolTip(tr("Volume handler: arrow Up/Down"));

    ui->labelRecordingTimer->setToolTip(tr("Timer for recording"));
    ui->tabMediaPlayer->setToolTip(tr("Media player tab"));
    ui->labelMediaElapsedTime->setToolTip(tr("Elapsed time of media playback"));
    ui->sliderMediaPlayback->setToolTip(tr("Slider for media playback"));
    ui->labelMediaTotalTime->setToolTip(tr("Total time of the media"));
    ui->sliderMediaPlayerVolume->setToolTip(tr("Adjust media volume: arrow Up/Down"));
    ui->labelMediaVolume->setToolTip(tr("Label for media volume control"));
    ui->comboBoxAudioSelector->setToolTip(tr("Select audio track for media: A"));

    ui->pushButtonFullScreen->setToolTip(tr("Fullscreen mode: F/Esc"));

}

// ---------------------------------------------------------------------------------
void MainWindow::setToImageCaptureMode()
{
    m_recorderButtonType = mApp::RecordingType::RECORD_TYPE_CAMERA;
    ui->pushButtonCaptureMedia->setIcon(QIcon(":/resource/captureImage.svg"));
    showCamera();

    ui->pushButtonCancelRec->setDisabled(true);
    ui->pushButtonCancelRec->setHidden(false);
    ui->labelRecordingTimer->setVisible(false);
    ui->labelRecordingTimer->setText("00:00:00");

    qDebug() << Q_FUNC_INFO << "Set to Image capture mode";
}
void MainWindow::setToVideoCaptureMode()
{
    m_recorderButtonType = mApp::RecordingType::RECORD_TYPE_VIDEO;
    ui->pushButtonCaptureMedia->setIcon(QIcon(":/resource/recordVideo.svg"));
    ui->pushButtonCancelRec->setDisabled(true);
    ui->pushButtonCancelRec->setHidden(true);
    ui->labelRecordingTimer->setVisible(true);

    qDebug() << Q_FUNC_INFO << "Set to Video capture mode";
}
void MainWindow::setToAudioCaptureMode()
{
    closeCamera();
    ui->vLayoutForCamera->addItem(new QSpacerItem(1,1,QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_recorderButtonType = mApp::RecordingType::RECORD_TYPE_AUDIO;
    ui->pushButtonCaptureMedia->setIcon(QIcon(":/resource/mic.svg"));

    ui->pushButtonCancelRec->setDisabled(true);
    ui->pushButtonCancelRec->setHidden(true);
    ui->labelRecordingTimer->setVisible(true);

    qDebug() << Q_FUNC_INFO << "Set to Audio capture mode";
}
//----------------------------------------------------------------------------------
void MainWindow::setRecStateRecording()
{
    m_multimediaRecordingState = mApp::RECORDING_ACTIVE;
    ui->pushButtonCaptureMedia->setIcon(QIcon(":/resource/pauseMedia.svg"));

    ui->pushButtonSaveMediaRec->setDisabled(false);
}
void MainWindow::setRecStatePaused()
{
    m_multimediaRecordingState = mApp::RECORDING_PAUSED;
    ui->pushButtonCaptureMedia->setIcon(QIcon(":/resource/playMedia.svg"));
}
void MainWindow::setRecStateStopped()
{
    if(m_multimediaRecordingState == mApp::RECORDING_STOPPED)
        return;

    ui->pushButtonSaveMediaRec->setDisabled(true);

    bool isVideoRec = m_recorderButtonType == mApp::RECORD_TYPE_VIDEO;

    QIcon captureButtonIcon(QString(":/resource/%1.svg").arg(isVideoRec ? "recordVideo":"mic"));
    ui->pushButtonCaptureMedia->setIcon(captureButtonIcon);

    m_multimediaRecordingState = mApp::RECORDING_STOPPED;

    const auto* recorder = isVideoRec ? m_videoRecorder:m_audioRecorder;
    ui->labelRecordingTimer->setText("00:00:00");

    if(recorder->recorderState() != QMediaRecorder::StoppedState)
        qWarning() << Q_FUNC_INFO << "Error recorder state mismatch.";
    else
        qInfo() << Q_FUNC_INFO << "";
}

void MainWindow::setRecState(mApp::RecordingState stateParam)
{
    switch (stateParam) {
    case mApp::RECORDING_STOPPED:{
        setRecStateStopped();
        break;
    }
    case mApp::RECORDING_PAUSED:{
        setRecStatePaused();
        break;
    }
    case mApp::RECORDING_ACTIVE:{
        setRecStateRecording();
        break;
    }
    default:
        break;
    }
}
// ----------------------------- ===========================


//////////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
    delete ui;
    qInfo() << Q_FUNC_INFO << "Leaving mainWindow ~destructor";
}

void MainWindow::logAboutAvailableMediaDevices()
{
    qDebug() << "Available Cameras:";
    for (const auto &camera : m_cameras)
        qDebug() << camera.description();

    qDebug() << "Available Microphones:";
    for (const auto &microphone : m_microphones)
        qDebug() << microphone.description();
}
void MainWindow::changeEvent(QEvent *event)
{
    Q_UNUSED(event)
    // QMainWindow::changeEvent(event);

    // if (event->type() == QEvent::WindowStateChange) {
    //     if (this->windowState() & Qt::WindowMinimized) {
    //         closeCamera(); // Call your function to close the camera
    //         qDebug() << "Window minimized. Closing the camera.";
    //     } else {
    //         setToImageCaptureMode();
    //     }
    // }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    m_mediaPlayerHandler->buttonHandler(event);
}






