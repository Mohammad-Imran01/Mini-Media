#include "imagecropper.h"

#include <QDebug>
#include <QPainter>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>


ImageCropper::ImageCropper(QWidget *parent)
    : QLabel(parent)
{
    setMouseTracking(true);
    setBackgroundRole(QPalette::Highlight);
}


ImageCropper::ImageCropper(ImageCropper *parentLabelParam, QRect showRectParam)
    : QLabel(parentLabelParam)
{
    setWindowFlag(Qt::Dialog);
    setAlignment(Qt::AlignCenter);  // Center align the pixmap
    setMouseTracking(true);

    // Ensure parent has a valid pixmap
    auto parentPixmap = parentLabelParam->pixmap();
    if (!parentPixmap || parentPixmap.isNull()) {
        qDebug() << "Parent ImageCropper does not have a valid pixmap.";
        return;
    }

    // Validate and crop the rectangle
    QRect validRect = showRectParam.intersected(parentPixmap.rect());
    if (!validRect.isValid()) {
        qDebug() << "Invalid crop rectangle. No pixmap set.";
        return;
    }

    // Crop and set the pixmap
    QPixmap croppedPixmap = parentPixmap.copy(validRect);  // Crop the pixmap
    setPixmap(croppedPixmap);  // Set the cropped pixmap

    // Create the layout for preview image and buttons
    QVBoxLayout *layoutPreviewImage = new QVBoxLayout(this);

    // Spacer above the image
    layoutPreviewImage->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Buttons layout below the image
    QHBoxLayout *layoutPreviewImageButtons = new QHBoxLayout();
    layoutPreviewImage->addLayout(layoutPreviewImageButtons);

    // Spacer to the left of buttons

    // "Save" button
    QPushButton
        *saveButton = new QPushButton("Save", this),
        *closeButton = new QPushButton("Close", this);

    connect(saveButton, &QPushButton::clicked, this, &ImageCropper::onSaveButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &ImageCropper::onCloseButtonClicked);


    // left spacer
    layoutPreviewImageButtons->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));

    //buttons on image previewed
    layoutPreviewImageButtons->addWidget(saveButton); // btn save
    layoutPreviewImageButtons->addWidget(closeButton);// btn close

    // right spacer
    layoutPreviewImageButtons->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

// Slot for Close Button
void ImageCropper::onCloseButtonClicked() {
    this->close();
}
// Slot for Save Button
void ImageCropper::onSaveButtonClicked() {
    QString savePath = QFileDialog::getSaveFileName(this, "Save Cropped Image", "", "Images (*.png *.jpg *.bmp)");
    if (!savePath.isEmpty()) {
        if (pixmap().save(savePath)) {
            QMessageBox::information(this, "Save", "Cropped image saved successfully!");
        } else {
            QMessageBox::warning(this, "Save", "Failed to save the cropped image.");
        }
    }
}

void ImageCropper::paintRectangle()
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // Optional: enables antialiasing for smooth lines

    // Set a very light fill color with some transparency (alpha = 0.1)
    painter.fillRect(m_RectSelected, QColor(0, 0, 177, 25));  // Light blue with alpha=25 (very light)

    // Set up a thick dashed (dotted) border
    QPen pen;
    pen.setColor(QColor(0, 0, 177, 100));  // Slightly darker blue with more opacity
    pen.setStyle(Qt::DashDotLine);  // Use dotted (dash-dot) line style
    pen.setWidth(2);  // Set a thicker border
    painter.setPen(pen);

    painter.drawRect(m_RectSelected);
}
void ImageCropper::showCroppedPreview()
{
    if (!m_previewLabel) {
        m_previewLabel = new ImageCropper(this, m_RectSelected);
        m_previewLabel->setWindowFlags(Qt::ToolTip);  // Makes it appear as a floating widget
        m_previewLabel->setAttribute(Qt::WA_DeleteOnClose, false);  // Keep it persistent
    }

    auto parentPixmap = pixmap();
    if (!parentPixmap.isNull()) {
        QRect validRect = m_RectSelected.intersected(parentPixmap.rect());
        if (validRect.isValid()) {
            QPixmap croppedPixmap = parentPixmap.copy(validRect);
            m_previewLabel->setPixmap(croppedPixmap.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    // Update the preview label's geometry and show it
    m_previewLabel->setGeometry(m_RectSelected.topRight().x(), m_RectSelected.topRight().y(),
                                m_RectSelected.width(), m_RectSelected.height());

    m_previewLabel->show();
}



void ImageCropper::mousePressEvent(QMouseEvent *ev)
{
    isPressed = true;
    m_PosStart = ev->pos();
}
void ImageCropper::mouseReleaseEvent(QMouseEvent *ev)
{
    if(!isPressed) {
        qDebug() << "Was not pressed";
        return;
    }

    m_PosEnd = ev->pos();

    m_RectSelected = QRect(m_PosStart, m_PosEnd);

    showCroppedPreview();

    isPressed = false;
    m_PosStart = m_PosEnd = QPoint();
    m_RectSelected = QRect();
}
void ImageCropper::mouseMoveEvent(QMouseEvent* event)
{
    if (isPressed) {
        // Check if left mouse button is pressed
        m_PosEnd = event->pos();
        m_RectSelected = QRect(m_PosStart, m_PosEnd);
        update();
    } else {
        // Left mouse button is not pressed, do nothing
    }
}
void ImageCropper::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event)
}
void ImageCropper::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);

    m_RectSelected = QRect(m_PosStart, m_PosEnd);

    if(!m_RectSelected.isValid()) {
        qDebug() << "Rect is too small";
        return;
    }

    paintRectangle();
    // showCroppedPreview();

    // Optional: Additional debug output
    qDebug() << Q_FUNC_INFO << "Returning from paintEvent";
}



void ImageCropper::printPos()
{
    qDebug() << "Start" << m_PosStart << ", end" << m_PosEnd;
}


