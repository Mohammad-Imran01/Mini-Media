#ifndef IMAGECROPPER_H
#define IMAGECROPPER_H

#include <QObject>
#include <QMouseEvent>
#include <QLabel>
#include <QPoint>
#include <QRect>
#include <QPaintEvent>

#define IMAGE_ZOOM_FACTOR

namespace mApp {
const int IMAGE_ZOOM_SCALE_FACTOR = 1;
}

class ImageCropper : public QLabel
{
    Q_OBJECT
public:
    explicit ImageCropper(QWidget *parent = nullptr);
    explicit ImageCropper(ImageCropper* image, QRect);

signals:
protected:
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent* ev);
    void paintEvent(QPaintEvent *event);
    void wheelEvent(QWheelEvent *event) override;

private:
    void printPos();
    void onCloseButtonClicked();
    void onSaveButtonClicked();
    void paintRectangle();
    void showCroppedPreview();

    bool isPressed = false;
    QPoint m_PosStart, m_PosEnd;
    QRect m_RectSelected;
    ImageCropper *m_previewLabel = nullptr;  // Preview label for the cropped region
};

#endif // IMAGECROPPER_H
