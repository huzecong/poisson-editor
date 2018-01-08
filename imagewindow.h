#ifndef POISSONEDITOR_IMAGEWINDOW_H
#define POISSONEDITOR_IMAGEWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "utils.h"

class ImageScene : public QGraphicsScene {
Q_OBJECT

public:
    ImageScene();

    void setPixmap(const QPixmap &pixmap);
    const QPainterPath *getSelection() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QPointF clampedPoint(const QPointF &point);
    bool inSelection = false, hasSelection = false;
    QPainterPath lassoPath;
    QSize imageSize;

    QGraphicsPathItem *pathItem;
    QGraphicsPixmapItem *imageItem = nullptr;
};

class ImageWindow : public QMainWindow {
Q_OBJECT

public:
    explicit ImageWindow(QWidget *parent);
    bool loadFile(const QString &filePath);
    const QString currentFile() const;
    const QString currentFileName() const;

    void showWithSizeHint(QSize parentSize);

    const bool hasSelection() const;
    const QPixmap * getSelectedImage();

private:
    double scale;
    void setSlider(double scale);

protected:
    bool event(QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);
    bool nativeGestureEvent(QNativeGestureEvent *event); // macOS-specific

private:
    void drawLineBresenham(utils::BitMatrix &mat, QPoint p0, QPoint p1) const;

    QSize imageSize;
    bool inGesture = false;
    double scaleBeforeGesture;
    double cumulativeScale;

    ImageScene *scene;
    QGraphicsView *view;
    QPixmap originalImage;

    QSlider *zoomSlider;
    QLabel *zoomScaleLabel;

    QPixmap selectedImage;
    QPainterPath *selectionPath = nullptr;
};

#endif //POISSONEDITOR_IMAGEWINDOW_H
