#ifndef MDICHILD_H
#define MDICHILD_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

class ImageWindow : public QMainWindow {
Q_OBJECT

public:
    explicit ImageWindow(QWidget *parent);
    bool loadFile(const QString &filePath);
    const QString currentFile() const;
    const QString currentFileName() const;

    void showWithSizeHint(QSize parentSize);

//public slots:
//    bool open();
//    bool saveAs();
//    void print();
//    void copy();
//    void paste();
//    void zoomIn();
//    void zoomOut();
//    void normalSize();
//    void fitToWindow();
//
private:
//    void updateActions();
//    bool saveFile(const QString &fileName);
    void setImage(const QImage &newImage);

    double scale;
    void setSlider(double scale);

    bool event(QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);
    bool nativeGestureEvent(QNativeGestureEvent *event); // macOS-specific
//    void scaleImage(double factor);
//    void adjustScrollBar(QScrollBar *scrollBar, double factor);

    QSize imageSize;
    bool inGesture;
    double scaleBeforeGesture;
    double cumulativeScale;

    QGraphicsScene* scene;
    QGraphicsView* view;
    QGraphicsPixmapItem *imageItem;

    QSlider *zoomSlider;
    QLabel *zoomScaleLabel;

};

#endif
