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
    ~ImageScene();
    ImageScene(const ImageScene &) = delete;
    ImageScene(ImageScene &&) = delete;
    ImageScene &operator =(const ImageScene &) = delete;
    ImageScene &operator =(ImageScene &&) = delete;

    void setPixmap(const QPixmap &pixmap);
    const QPainterPath *getSelection() const;
    void clearSelection();
    void pastePixmap(const QPixmap &pixmap);
    const QList<QGraphicsPixmapItem *> &getPastedPixmaps() const;
    QPixmap getSelectedImage();

    void poissonFusion();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    void drawLineBresenham(utils::BitMatrix &mat, QPoint p0, QPoint p1) const;

    QPointF clampedPoint(const QPointF &point);
    QPixmap pixmap;
    QSize imageSize;
    QGraphicsPixmapItem *imageItem = nullptr;

    bool inLassoSelection = false, hasLassoSelection = false;
    QPainterPath lassoPath;
    QGraphicsPathItem *pathItem;
    QPen *pathPen;
    QVariantAnimation *pathBorderAnimation;

    QPixmap selectedImage;
    QPainterPath *selectionPath = nullptr;

    bool inItemSelection = false;
    QGraphicsItem *selectedItem = nullptr;
    QPointF selectionPosDelta;
    QGraphicsRectItem *selectionBox = nullptr;

    QList<QGraphicsPixmapItem *> pastedPixmaps;
    float maxZValue = 2.0;
};

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

private:
    double scale;
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
