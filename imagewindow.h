#ifndef POISSONEDITOR_IMAGEWINDOW_H
#define POISSONEDITOR_IMAGEWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "utils.h"
#include "imagescene.h"


class ImageWindow : public QMainWindow {
Q_OBJECT

public:
    explicit ImageWindow(QWidget *parent);
    ~ImageWindow();
    ImageWindow(const ImageWindow &) = delete;
    ImageWindow(ImageWindow &&) = delete;
    ImageWindow &operator =(const ImageWindow &) = delete;
    ImageWindow &operator =(ImageWindow &&) = delete;

    bool loadFile(const QString &filePath);
    const QString currentFile() const;
    const QString currentFileName() const;

    void showWithSizeHint(QSize parentSize);

    const bool hasSelection() const;
    const bool hasPastedPixmaps() const;
    QPixmap getSelectedImage();
    void pastePixmap(const QPixmap &pixmap);

    void poissonFusion();
    void smartFill();

private:
    double scale = 1.0;
    void setSlider(double scale);

protected:
    bool event(QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);
    bool nativeGestureEvent(QNativeGestureEvent *event); // macOS-specific

private:
    QSize imageSize;
    bool inGesture = false;
    double scaleBeforeGesture;
    double cumulativeScale;

    ImageScene *scene;
    QGraphicsView *view;
    QPixmap originalImage;

    QSlider *zoomSlider;
    QLabel *zoomScaleLabel;
};

#endif //POISSONEDITOR_IMAGEWINDOW_H
