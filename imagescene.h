#ifndef POISSONEDITOR_IMAGESCENE_H
#define POISSONEDITOR_IMAGESCENE_H


#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include "utils.h"
#include "bitmatrix.h"


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
    void smartFill();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void drawLineBresenham(utils::Matrix<bool> &mat, QPoint p0, QPoint p1) const;
    BitMatrix getMaskFromPath(const QPainterPath &path);
    QPointF clampedPoint(const QPointF &point);
    void eraseLassoSelection();

    QPixmap pixmap;
    QImage originalImage;
    QSize imageSize;
    QGraphicsPixmapItem *imageItem = nullptr;

    bool inLassoSelection = false, hasLassoSelection = false;
    QPainterPath lassoPath;
    QGraphicsPathItem *pathItem;
    QPen *pathPen;
    QVariantAnimation *pathBorderAnimation;

    BitMatrix *bgAlpha = nullptr;

    QPixmap selectedImage;
    QPainterPath *selectionPath = nullptr;

    bool inItemMovement = false;
    QGraphicsPixmapItem *selectedItem = nullptr;
    QPointF selectionPosDelta;
    QGraphicsRectItem *selectionBox = nullptr;

    QList<QGraphicsPixmapItem *> pastedPixmaps;
    float maxZValue = 2.0;
};


#endif //POISSONEDITOR_IMAGESCENE_H
