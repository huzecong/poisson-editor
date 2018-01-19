//
// Created by Kanari on 2018/1/19.
//

#ifndef POISSONEDITOR_IMAGESCENE_H
#define POISSONEDITOR_IMAGESCENE_H


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
    void keyPressEvent(QKeyEvent *event) override;

private:
    void drawLineBresenham(utils::BitMatrix &mat, QPoint p0, QPoint p1) const;

    QPointF clampedPoint(const QPointF &point);
    QPixmap pixmap;
    QImage originalImage;
    QSize imageSize;
    QGraphicsPixmapItem *imageItem = nullptr;

    bool inLassoSelection = false, hasLassoSelection = false;
    QPainterPath lassoPath;
    QGraphicsPathItem *pathItem;
    QPen *pathPen;
    QVariantAnimation *pathBorderAnimation;

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
