#include "mediaplayer.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QFileInfo>
#include <QString>
#include <QLabel>
#include <QImage>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMediaMetaData>
#include <QMediaFormat>

#include "mainwindow.h"
#include "src/common/imagecropper.h"

#include "ui_mainwindow.h"

MediaPlayer::MediaPlayer(MainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_imageLabel(new ImageCropper(mainWindow))
    , m_audioOutput(new QAudioOutput(mainWindow))
    , m_videoWidget(new QVideoWidget(mainWindow))
    , m_mediaPlayer(new QMediaPlayer(mainWindow))
{
    mainUi = m_mainWindow->getMainUi();

    m_vLayoutMediaPlayer = mainUi->vLayoutMediaPlayer;

    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setVideoOutput(m_videoWidget);

    m_videoWidget->hide();
    m_imageLabel->hide();

    connectSlots();

    setMediaPlayerNoMediaState();
    qDebug() << Q_FUNC_INFO << "Leaving MediaPlayer constructor";
}

MediaPlayer::~MediaPlayer() {
    if(m_videoWidget) {
        delete m_videoWidget;
        m_videoWidget = nullptr;
    }
    qInfo() << Q_FUNC_INFO << "Leaving MediaPlayer destructor";
}

void MediaPlayer::buttonHandler(QKeyEvent *event)
{
    if(mainUi->mainTabWidget->currentIndex() != static_cast<int>(mApp::TAB_MEDIA_PLAYER))
        return;

    const auto _key = event->key();

    if (_key == Qt::Key_O) {
        if(mainUi->pushButtonLoadMedia->isEnabled())
            loadMedia();
        return;
    }

    if (m_renderingType != mApp::Rendering_Video)
        return;

    if(_key == Qt::Key_Escape || _key == Qt::Key_F)
        goFullScreenVideo(event);
    else if (_key == Qt::Key_A)
        handleNextAudioTrack();
    else if (_key == Qt::Key_Space)
        handleMediaPlayerToggleButton();
    else if (_key == Qt::Key_Right)
        handleNextPressed();
    else if (_key == Qt::Key_Left)
        handlePrevPressed();
    else if (_key == Qt::Key_Down) {
        const int currentValue = mainUi->sliderMediaPlayerVolume->value();
        const int newValue = qMax(currentValue - 5, 0); // Make sure it doesn't go below 0
        mainUi->sliderMediaPlayerVolume->setValue(newValue);
    }
    else if (_key == Qt::Key_Up) {
        const int currentValue = mainUi->sliderMediaPlayerVolume->value();
        const int newValue = qMin(currentValue + 5, 100); // Make sure it doesn't go above 100
        mainUi->sliderMediaPlayerVolume->setValue(newValue);
    } else if (_key == Qt::Key_M) {
        handleMute();
    }
    if((event->modifiers() & Qt::CTRL) && (_key == Qt::Key_R)) {
        if(mainUi->pushButtonMediaRestart->isEnabled()) {
            mainUi->pushButtonMediaRestart->click();
        }
    }

}


void MediaPlayer::connectSlots()
{
    connect(mainUi->mainTabWidget, &QTabWidget::currentChanged, this, &MediaPlayer::handleTabChanged);

    connect(mainUi->sliderMediaPlayback, &QSlider::valueChanged, this, &MediaPlayer::handlePlaybackSlider);
    connect(mainUi->sliderMediaPlayerVolume, &QSlider::valueChanged, this, &MediaPlayer::handleVolumeSlider);

    connect(mainUi->pushButtonMediaNext, &QPushButton::clicked, this, &MediaPlayer::handleNextPressed);
    connect(mainUi->pushButtonMediaPrev, &QPushButton::clicked, this, &MediaPlayer::handlePrevPressed);
    connect(mainUi->pushButtonMediaRestart, &QPushButton::clicked, this, &MediaPlayer::handleRestartPressed);
    connect(mainUi->pushButtonToggleMedia, &QPushButton::clicked, this, &MediaPlayer::handleMediaPlayerToggleButton);
    connect(mainUi->pushButtonLoadMedia, &QPushButton::clicked, this, &MediaPlayer::loadMedia);
    connect(mainUi->pushButtonSound, &QPushButton::clicked, this, &MediaPlayer::handleMute);

    connect(mainUi->pushButtonFullScreen, &QPushButton::clicked, this, [this](){
        if (m_renderingType != mApp::Rendering_Video)
            return;
        if (!m_mainWindow->isFullScreen())
            setFullScreen();
        else
            unsetFullScreen();
    });

    // Connect ComboBox activation to change the audio track
    connect(mainUi->comboBoxAudioSelector, QOverload<int>::of(&QComboBox::activated), this, [this](int idx) {
        m_mediaPlayer->setActiveAudioTrack(idx);
        qDebug() << Q_FUNC_INFO << "Selected Audio Track Index:" << idx;
    });

    // Duration changed handler
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [this]() {
        if (!m_mediaPlayer) {
            qWarning() << Q_FUNC_INFO << "Media player is null!";
            return;
        }

        const qint64 duration = m_mediaPlayer->duration();
        const QString formattedTime = formatTime(duration);

        mainUi->labelMediaTotalTime->setText(formattedTime);
    });

    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        Q_UNUSED(status)
        qint64 totalDuration = m_mediaPlayer->duration();
        mainUi->labelMediaTotalTime->setText(formatTime(totalDuration));
        qDebug() << Q_FUNC_INFO << "Played total time: " << m_mediaPlayer->duration();
    });

    mainUi->comboBoxAudioSelector->setFocusPolicy(Qt::NoFocus);

    // Position changed handler
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, [this]() {
        if (!m_mediaPlayer) {
            qWarning() << Q_FUNC_INFO << "Media player is null!";
            return;
        }

        const qint64 position = m_mediaPlayer->position();
        const QString formattedTime = formatTime(position);

        mainUi->labelMediaElapsedTime->setText(formattedTime);
    });


    // connect(m_imageLabel, QLabel::mousePressEvent, this, [this](QMouseEvent* event){
    //     if(event->type() == QMouseEvent::pressed) {
    //         qDebug() << "Pressed: " << event->pos();
    //     }
    // });
    // connect(m_imageLabel, QLabel::mouseReleaseEvent, this, [this](QMouseEvent* event){
    //     if(event->type() == QMouseEvent::released) {
    //         qDebug() << "Released: " << event->pos();
    //     }
    // });
}

void MediaPlayer::loadImage(const QString &filePath)
{
    QImage image(filePath);
    if (image.isNull()) {
        qDebug() << "Failed to load image from:" << filePath;
        return;
    }

    qDebug() << Q_FUNC_INFO << "Image loading started";

    // Set the pixmap of the image label with the scaled image
    m_imageLabel->setPixmap(QPixmap::fromImage(image).scaled(
        mainUi->tabMediaPlayer->geometry().size(), // Scale to fit the QGroupBox
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation // Smoother scaling
        ));
    m_imageLabel->setAlignment(Qt::AlignCenter); // Center the image within the label

    stopMediaPlayer();

    showImageWidget();

    setMediaPlayerLoadedImageState();
    m_renderingType = mApp::Rendering_Image;

    qDebug() << Q_FUNC_INFO << "Image loaded successfully:" << filePath;
}

void MediaPlayer::playMedia(const QString &filePath)
{
    if (!m_mediaPlayer) {
        qWarning() << Q_FUNC_INFO << "Media player unavailable!";
        return;
    }

    // Stop current playback if needed
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState ||
        m_mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        m_mediaPlayer->stop();
        qDebug() << Q_FUNC_INFO << "Stopped currently playing media.";
    }

    showNoneWidget();

    QUrl mediaUrl = QUrl::fromLocalFile(filePath);
    if (!mediaUrl.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid file path provided:" << filePath;
        return;
    }
    m_mediaPlayer->setSource(mediaUrl);
    showVideoWidget();


    // Set volume slider to reflect the current volume
    if (m_mediaPlayer->audioOutput()) {
        const int currentVolume = static_cast<int>(m_mediaPlayer->audioOutput()->volume() * 100);
        mainUi->sliderMediaPlayerVolume->setValue(currentVolume);
    } else {
        qWarning() << Q_FUNC_INFO << "Audio output is unavailable!";
    }

    // Set position slider to reflect the current playback position
    if (m_mediaPlayer) {
        const qint64 duration = m_mediaPlayer->duration(); // Total duration in ms
        if (duration > 0) {
            const int currentPos = static_cast<int>((m_mediaPlayer->position() * 100) / duration);
            mainUi->sliderMediaPlayback->setValue(currentPos);
        } else {
            mainUi->sliderMediaPlayback->setValue(0); // Reset slider if no duration is available
        }
    } else {
        qWarning() << Q_FUNC_INFO << "Media player is unavailable!";
    }


    // Play the media
    m_mediaPlayer->play();

    // Retrieve available audio tracks
    mainUi->comboBoxAudioSelector->clear();
    QList<QMediaMetaData> audiosAvailable = m_mediaPlayer->audioTracks();

    if(audiosAvailable.length() > 1) {
        for (const QMediaMetaData &audioMetaData : audiosAvailable) {
            QString language = audioMetaData.value(QMediaMetaData::Language).toString();
            if (language.isEmpty()) {
                language = "Unknown Language";
            }
            qDebug() << Q_FUNC_INFO << "Audio Track Language:" << language;
            mainUi->comboBoxAudioSelector->addItem(language);
        }
    }

    bool hasComboBox = mainUi->comboBoxAudioSelector->count() > 1;
    mainUi->comboBoxAudioSelector->setEnabled(hasComboBox);
    mainUi->comboBoxAudioSelector->setVisible(hasComboBox);


    setMediaPlayerPlayingState();

    qDebug() << Q_FUNC_INFO << "Playing media:" << filePath;
}


void MediaPlayer::pauseMediaPlayer()
{
    if (!m_mediaPlayer) {
        qWarning() << Q_FUNC_INFO << "Media player unavailable!";
        return;
    }

    if(!m_mediaPlayer->isPlaying()) {
        qDebug() << Q_FUNC_INFO << "Not in playing state";
        return;
    }

    setMediaPlayerPausedState();

    m_mediaPlayer->pause();
}

void MediaPlayer::resumeMediaPlayer()
{
    if (!m_mediaPlayer) {
        qWarning() << Q_FUNC_INFO << "Media player unavailable!";
        return;
    }
    setMediaPlayerPlayingState();

    m_mediaPlayer->play();
}

void MediaPlayer::stopMediaPlayer()
{
    if (!m_mediaPlayer) {
        qWarning() << Q_FUNC_INFO << "Media player unavailable!";
        return;
    }

    // Stop playback and reset the media player
    m_mediaPlayer->stop();

    // Clear the media source
    m_mediaPlayer->setSource(QUrl());

    setMediaPlayerNoMediaState();
    qDebug() << Q_FUNC_INFO << "Media player stopped and resources unloaded.";
}

void MediaPlayer::handleMute()
{
    const bool muted = m_audioOutput->isMuted();
    if(muted) {
        mainUi->pushButtonSound->setIcon(QIcon(":/resource/sound.svg"));
        m_audioOutput->setMuted(false);
    } else {
        m_audioOutput->setMuted(true);
        mainUi->pushButtonSound->setIcon(QIcon(":/resource/mute.svg"));
    }
}
//------------------------------------------
void MediaPlayer::showNoneWidget()
{
    if (!m_vLayoutMediaPlayer) {
        qWarning() << Q_FUNC_INFO << "Invalid layout parameter!";
        return;
    }

    // Reset media labels
    mainUi->labelMediaElapsedTime->setText("00:00:00");
    mainUi->labelMediaTotalTime->setText("00:00:00");
    mainUi->labelMediaVolume->setText("00:00:00");


    // Remove and delete all widgets from the layout
    QLayoutItem* child;
    while ((child = m_vLayoutMediaPlayer->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->hide();  // Hide the widget
            m_vLayoutMediaPlayer->removeWidget(child->widget());  // Remove from layout
        }
        delete child;  // Free the QLayoutItem memory
    }

    // Reset the rendering type to None
    m_renderingType = mApp::Rendering_None;

    qDebug() << Q_FUNC_INFO << "Player widget cleaned.";
}

void MediaPlayer::showWidget(QWidget* widget)
{
    showNoneWidget();  // Clean any existing widgets

    if (widget) {
        m_vLayoutMediaPlayer->addWidget(widget);
        widget->show();
    }

    mainUi->tabMediaPlayer->update();
    m_vLayoutMediaPlayer->update();
}

void MediaPlayer::showImageWidget()
{
    showWidget(m_imageLabel);  // Show image widget
}

void MediaPlayer::showAudioWidget()
{
    showWidget(nullptr);  // Placeholder for audio widget (you can add audio-specific widget if needed)
}

void MediaPlayer::showVideoWidget()
{
    showWidget(m_videoWidget);
}

//------------------------------------------

void MediaPlayer::loadMedia()
{
    QString filePath = QFileDialog::getOpenFileName(
        m_mainWindow,
        tr("Load Media"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
        tr("Media Files (*.png *.jpg *.jpeg *.bmp *.mp3 *.wav *.mp4 *.avi *.mkv);;Images (*.png *.jpg *.jpeg *.bmp);;Audio (*.mp3 *.wav);;Video (*.mp4 *.avi *.mkv)")
        );

    if (filePath.isEmpty()) {
        qDebug() << "Load operation cancelled.";
        return;
    }

    QFileInfo fileInfo(filePath);
    QString fileSuffix = fileInfo.suffix().toLower();

    if (fileSuffix == "png" || fileSuffix == "jpg" || fileSuffix == "jpeg" || fileSuffix == "bmp") {
        loadImage(filePath);
    } else if (fileSuffix == "mp3" || fileSuffix == "wav") {
        playMedia(filePath);
        m_renderingType = mApp::Rendering_Audio;
    } else if(fileSuffix == "mp4" || fileSuffix == "avi" || fileSuffix == "mkv") {
        playMedia(filePath);
        m_renderingType = mApp::Rendering_Video;
    } else {
        stopMediaPlayer();
        m_renderingType = mApp::Rendering_None;
        qDebug() << "Unsupported file type.";
    }
}

// ------------------------------------------------------------------------
void MediaPlayer::setMediaPlayerNoMediaStateInternal()
{
    if(!mainUi) {
        qWarning() << Q_FUNC_INFO << "Main Ui Unavailable";
        return;
    }
    mainUi->sliderMediaPlayback->setDisabled(true);
    mainUi->sliderMediaPlayerVolume->setDisabled(true);

    mainUi->pushButtonMediaPrev->setDisabled(true);
    mainUi->pushButtonToggleMedia->setDisabled(true);
    mainUi->pushButtonMediaNext->setDisabled(true);
    mainUi->pushButtonMediaRestart->setDisabled(true);
    mainUi->pushButtonSound->setDisabled(true);
    mainUi->pushButtonFullScreen->setDisabled(true);

    mainUi->pushButtonToggleMedia->setIcon(QIcon(":/resource/playMedia.svg"));
    //     // Clear all existing widgets in the layout

    m_imageLabel->hide();
    m_vLayoutMediaPlayer->removeWidget(m_imageLabel);
    m_videoWidget->hide();
    m_vLayoutMediaPlayer->removeWidget(m_videoWidget);


    mainUi->comboBoxAudioSelector->setEnabled(false);
    mainUi->comboBoxAudioSelector->setVisible(false);

    hideCodecButton();
}

void MediaPlayer::setMediaPlayerLoadedImageState()
{
    mainUi->sliderMediaPlayback->setDisabled(true);
    mainUi->sliderMediaPlayerVolume->setDisabled(true);

    mainUi->pushButtonMediaPrev->setDisabled(true);
    mainUi->pushButtonToggleMedia->setDisabled(true);
    mainUi->pushButtonMediaNext->setDisabled(true);
    mainUi->pushButtonMediaRestart->setDisabled(true);
    mainUi->pushButtonSound->setDisabled(true);
    mainUi->pushButtonFullScreen->setDisabled(true);

    hideCodecButton();

    mainUi->pushButtonToggleMedia->setIcon(QIcon(":/resource/playMedia.svg"));
}

void MediaPlayer::setMediaPlayerPlayingState()
{
    mainUi->sliderMediaPlayback->setDisabled(false);
    mainUi->sliderMediaPlayerVolume->setDisabled(false);
    mainUi->pushButtonSound->setDisabled(false);

    mainUi->pushButtonMediaRestart->setDisabled(false);
    mainUi->pushButtonMediaPrev->setDisabled(false);
    mainUi->pushButtonToggleMedia->setDisabled(false);
    mainUi->pushButtonMediaNext->setDisabled(false);
    mainUi->pushButtonFullScreen->setDisabled(false);


    showCodecButton();

    mainUi->pushButtonToggleMedia->setIcon(QIcon(":/resource/pauseMedia.svg"));
}

void MediaPlayer::setMediaPlayerPausedState()
{    
    mainUi->pushButtonToggleMedia->setIcon(QIcon(":/resource/playMedia.svg"));
}
// ------------------------------------------------------------------------

void MediaPlayer::handleNextPressed()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState ||
        m_mediaPlayer->playbackState() == QMediaPlayer::PausedState) {

        mainUi->sliderMediaPlayback->setValue(
            mainUi->sliderMediaPlayback->value() + mainUi->sliderMediaPlayback->singleStep());
    }
}
void MediaPlayer::handlePrevPressed()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState ||
        m_mediaPlayer->playbackState() == QMediaPlayer::PausedState) {

        mainUi->sliderMediaPlayback->setValue(
            mainUi->sliderMediaPlayback->value() - mainUi->sliderMediaPlayback->singleStep());
    }
}
void MediaPlayer::handleRestartPressed()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState ||
        m_mediaPlayer->playbackState() == QMediaPlayer::PausedState) {
        mainUi->sliderMediaPlayback->setValue(mainUi->sliderMediaPlayback->minimum());
    } else {
        qDebug() << Q_FUNC_INFO << "Cant restart";
    }
}

void MediaPlayer::handleButtonCodec()
{
    // if (!m_mediaPlayer) {
    //     qDebug() << "MediaPlayer not initialized.";
    //     return;
    // }
    // m_mediaPlayer->pause();


    // // Get the current media format (optional if you want to check current settings)
    // QMediaFormat currentFormat = m_mediaPlayer->;
    // m_mediaPlayer->audioOutput()->
    // // Create a new media format for toggling
    // QMediaFormat newFormat;

    // if (currentFormat.videoCodec() == QMediaFormat::VideoCodec::Unspecified) {
    //     // Currently in software mode, switch to hardware
    //     newFormat.setVideoCodec(QMediaFormat::VideoCodec::H264); // Example for HW decoding
    //     qDebug() << "Switching to hardware decoding.";
    // } else {
    //     // Currently in hardware mode, switch to software
    //     newFormat.setVideoCodec(QMediaFormat::VideoCodec::Any); // Software decoding
    //     qDebug() << "Switching to software decoding.";
    // }

    // // Apply the new format
    // m_mediaPlayer->(newFormat);

    // // Reload the media to apply the changes (optional, depends on your implementation)
    // m_mediaPlayer->setSource(m_mediaPlayer->source());
    // m_mediaPlayer->play();
}



void MediaPlayer::handleMediaPlayerToggleButton()
{
    if(m_mediaPlayer) {
        if(m_mediaPlayer->isPlaying())
            pauseMediaPlayer();
        else
            resumeMediaPlayer();
    }
}

void MediaPlayer::handleTabChanged(int currentTab)
{
    if(currentTab == static_cast<int>(mApp::TAB_MEDIA_RECORDER)) {
        switch (m_renderingType) {
        case mApp::Rendering_None: {
            // qDebug() << Q_FUNC_INFO << "No media shown.";
            // break;
        }
        case mApp::Rendering_Image:{
            break; // let the image be there, no issues;
        }
        case mApp::Rendering_Audio:{}
        case mApp::Rendering_Video:{
            pauseMediaPlayer();
            break;
        }
        default:
            break;
        }
    } else {
        switch (m_renderingType) {
        case mApp::Rendering_None: {
            break;
        }
        case mApp::Rendering_Image:{
            break; // let the image be there, no issues;
        }
        case mApp::Rendering_Audio:{
            // break;
        }
        case mApp::Rendering_Video:{
            // pauseMediaPlayer();
            break;
        }
        default:
            break;
        }
    }
}

void MediaPlayer::handlePlaybackSlider(int valParam)
{
    if (!m_mediaPlayer) {
        qWarning() << Q_FUNC_INFO << "Media player unavailable!";
        return;
    }

    const qint64 length = m_mediaPlayer->duration();
    if (length > 0) {
        // Calculate the media position based on slider value
        qint64 newPosition = (length * valParam) / 100;
        m_mediaPlayer->setPosition(newPosition);

        qDebug() << Q_FUNC_INFO << "Slider moved to" << valParam << "%, setting media position to" << newPosition;
    }
}

void MediaPlayer::handleVolumeSlider(int valParam)
{
    if (!m_mediaPlayer || !m_mediaPlayer->audioOutput()) {
        qWarning() << Q_FUNC_INFO << "Media player or audio output unavailable!";
        return;
    }

    const float updatedNormalizedVolume = valParam / 100.0f;
    m_mediaPlayer->audioOutput()->setVolume(updatedNormalizedVolume);

    mainUi->labelMediaVolume->setText(QString::number(valParam) + "%");


    qDebug() << Q_FUNC_INFO << "Slider moved to" << valParam << "%, setting volume to" << updatedNormalizedVolume;
}

void MediaPlayer::handleNextAudioTrack()
{
    if (m_renderingType != mApp::Rendering_Video)
        return;
    if (mainUi->comboBoxAudioSelector->count() > 1) {
        const int currentIndex = mainUi->comboBoxAudioSelector->currentIndex();
        const int nextIndex = (currentIndex + 1) % mainUi->comboBoxAudioSelector->count();
        mainUi->comboBoxAudioSelector->setCurrentIndex(nextIndex);
        m_mediaPlayer->setActiveAudioTrack(nextIndex);
        qDebug() << Q_FUNC_INFO << "Current audio track:" << mainUi->comboBoxAudioSelector->itemText(nextIndex);
    }
}

void MediaPlayer::setFullScreen()
{
    mainUi->controllerWidgetContainer->hide();
    m_mainWindow->showFullScreen();

    QWidget *currentCentralWidget = m_mainWindow->centralWidget();
    if (currentCentralWidget) {
        currentCentralWidget->setParent(nullptr);
    }

    m_videoWidget->setParent(nullptr);
    m_mainWindow->setCentralWidget(m_videoWidget);
}

void MediaPlayer::unsetFullScreen()
{
    mainUi->controllerWidgetContainer->show();

    QWidget *currentCentralWidget = m_mainWindow->centralWidget();
    if (currentCentralWidget) {
        currentCentralWidget->setParent(nullptr);
    }

    m_mainWindow->setCentralWidget(mainUi->centralwidget);
    m_videoWidget->setParent(m_mainWindow);
    m_vLayoutMediaPlayer->addWidget(m_videoWidget);
    m_mainWindow->showNormal();
}

void MediaPlayer::hideCodecButton()
{
    mainUi->spacerBeforeCodec->changeSize(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    mainUi->pushButtonAudioCodec->setEnabled(false);
    mainUi->pushButtonAudioCodec->setVisible(false);
}

void MediaPlayer::showCodecButton()
{
    mainUi->pushButtonAudioCodec->setEnabled(true);
    mainUi->pushButtonAudioCodec->setVisible(true);
    mainUi->spacerBeforeCodec->changeSize(40, 20, QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void MediaPlayer::goFullScreenVideo(QKeyEvent* event) {
    if (m_renderingType != mApp::Rendering_Video)
        return;

    if (!m_mainWindow->isFullScreen() && event->key() == Qt::Key_F)
        setFullScreen();
    else if(m_mainWindow->isFullScreen() && (event->key() == Qt::Key_Escape || event->key() == Qt::Key_F))
        unsetFullScreen();
}


QString MediaPlayer::formatTime(qint64 ms) const
{
    const qint64 sec = (ms / 1000) % 60;
    const qint64 min = (ms / (1000 * 60)) % 60;
    const qint64 hr = ms / (1000 * 60 * 60);

    return QTime(hr, min, sec).toString();
}


