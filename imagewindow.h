#ifndef POISSONEDITOR_IMAGEWINDOW_H
#define POISSONEDITOR_IMAGEWINDOW_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

class ImageScene : public QGraphicsScene {
Q_OBJECT

public:
    ImageScene();

    void setImage(const QImage &image);
    const QPainterPath *getSelection() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QPointF clampedPoint(const QPointF &point);
    bool inSelection, hasSelection;
    QPainterPath lassoPath;
    QSize imageSize;

    QGraphicsPathItem *pathItem;
    QGraphicsPixmapItem *imageItem;
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
    void copy();
    void paste();
    void cut();

private:
    double scale;
    void setSlider(double scale);

protected:
    bool event(QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);
    bool nativeGestureEvent(QNativeGestureEvent *event); // macOS-specific

private:
    QSize imageSize;
    bool inGesture;
    double scaleBeforeGesture;
    double cumulativeScale;

    ImageScene *scene;
    QGraphicsView *view;

    QSlider *zoomSlider;
    QLabel *zoomScaleLabel;

};

#endif //POISSONEDITOR_IMAGEWINDOW_H
