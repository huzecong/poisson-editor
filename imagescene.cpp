//
// Created by Kanari on 2018/1/19.
//

#include "imagescene.h"
#include "poissonsolver.h"

ImageScene::ImageScene() {
    pathItem = new QGraphicsPathItem;
    pathPen = new QPen(Qt::black, 2, Qt::DashLine);
    pathPen->setDashPattern({4.0, 4.0});
    pathItem->setPen(*pathPen);
    pathBorderAnimation = new QVariantAnimation();
    pathBorderAnimation->setStartValue(0.0);
    pathBorderAnimation->setEndValue(8.0);
    pathBorderAnimation->setDuration(1000);
    pathBorderAnimation->setLoopCount(-1);
    connect(pathBorderAnimation, &QVariantAnimation::valueChanged, [&](const QVariant &value) {
//        qDebug() << value.toReal();
        pathPen->setDashOffset(value.toReal());
        pathItem->setPen(*pathPen);
    });
    pathBorderAnimation->start();
    pathItem->setBrush(QBrush(QColor(0, 100, 200, 50))); // half-transparent light blue
    pathItem->setZValue(1); // put on top of everything else
    addItem(pathItem);
}

ImageScene::~ImageScene() {
    delete pathBorderAnimation;
    delete pathPen;
    delete pathItem;
    delete imageItem;
    delete bgAlpha;
}

void ImageScene::setPixmap(const QPixmap &pixmap) {
    if (imageItem != nullptr)
        removeItem(imageItem);

    setSceneRect(pixmap.rect());
    this->pixmap = pixmap;
    originalImage = pixmap.toImage();
    imageSize = pixmap.size();

    imageItem = new QGraphicsPixmapItem(pixmap);
    imageItem->setZValue(0);
    imageItem->setTransformationMode(Qt::SmoothTransformation);
    addItem(imageItem);

    bgAlpha = new BitMatrix(imageSize.width(), imageSize.height());
}

const QPainterPath *ImageScene::getSelection() const {
    return hasLassoSelection ? &lassoPath : nullptr;
}

QPointF ImageScene::clampedPoint(const QPointF &point) {
    auto pos = point;
    pos.setX(utils::clamp(pos.x(), 0.0, imageSize.width() - 1.0));
    pos.setY(utils::clamp(pos.y(), 0.0, imageSize.height() - 1.0));
    return pos;
}

void ImageScene::clearSelection() {
    // Cancel item selection
    selectedItem = nullptr;
    if (selectionBox != nullptr) {
        removeItem(selectionBox);
        delete selectionBox;
        selectionBox = nullptr;
    }
    // Clear lasso path
    lassoPath = QPainterPath();
    pathItem->setPath(lassoPath);
}

void ImageScene::pastePixmap(const QPixmap &pixmap) {
    auto *item = new QGraphicsPixmapItem(pixmap);
    item->setPos((imageSize.width() - pixmap.width()) / 2, (imageSize.height() - pixmap.height()) / 2);
    maxZValue += 0.1;
    item->setZValue(maxZValue);
    addItem(item);
    pastedPixmaps.append(item);
    clearSelection();
}

const QList<QGraphicsPixmapItem *> &ImageScene::getPastedPixmaps() const {
    return pastedPixmaps;
}

void ImageScene::poissonFusion() {
    clearSelection();

    // Sort patches by ascending z-value
    std::sort(pastedPixmaps.begin(), pastedPixmaps.end(), [&](const QGraphicsPixmapItem *a, const QGraphicsPixmapItem *b) {
        return a->zValue() < b->zValue();
    });
    // Align pixmaps to pixel edge
    for (auto *item : pastedPixmaps)
        item->setPos(item->pos().toPoint());

    // Render the current scene to pixmap
    QImage image(imageSize, QImage::Format_ARGB32);
    QPainter imagePainter(&image);
    render(&imagePainter);

    // Create segmentation mask
    QImage mask(imageSize, QImage::Format_Grayscale8);
    mask.fill(0);
    QPainter maskPainter(&mask);
    int index = 0;
    for (auto *item : pastedPixmaps) {
        ++index;
//        index += 50;
        QPixmap patch(item->pixmap().size());
        patch.fill(QColor::fromHsv(0, 0, index));
        patch.setMask(item->pixmap().mask());
        maskPainter.drawPixmap(item->pos(), patch);
    }
    auto fusedImage = PoissonSolver::poissonFusion(originalImage, image, mask);
    pixmap = QPixmap::fromImage(fusedImage);

    for (auto *item : pastedPixmaps)
        removeItem(item);
    pastedPixmaps.clear();
    imageItem->setPixmap(pixmap);
}

void ImageScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton && imageItem != nullptr) {
        auto *item = itemAt(event->scenePos(), {});
        if (item == nullptr || item == imageItem || item == pathItem) {
            // Did not select item, start drawing path
            inLassoSelection = true;
            lassoPath = QPainterPath(clampedPoint(event->scenePos()));
            lassoPath.setFillRule(Qt::WindingFill);
            pathItem->setPath(lassoPath);
            // Cancel item selection
            selectedItem = nullptr;
            if (selectionBox != nullptr) {
                removeItem(selectionBox);
                delete selectionBox;
                selectionBox = nullptr;
            }
        } else {
            // Selected item, start moving item
            inItemMovement = true;
            selectionPosDelta = item->pos() - event->scenePos();
            if (item->type() != QGraphicsRectItem::Type && selectedItem != item) {
                selectedItem = dynamic_cast<QGraphicsPixmapItem *>(item);
                maxZValue += 0.1;
                item->setZValue(maxZValue);
                if (selectionBox != nullptr) {
                    removeItem(selectionBox);
                    delete selectionBox;
                }
                selectionBox = addRect(item->boundingRect(), QPen(Qt::black, 2, Qt::DashLine));
                selectionBox->setAcceptedMouseButtons(0);
                selectionBox->setPos(item->pos());
                selectionBox->setZValue(maxZValue);
            }
            // Clear lasso path
            lassoPath = QPainterPath();
            pathItem->setPath(lassoPath);
        }
    }
}

void ImageScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (inLassoSelection) {
        lassoPath.lineTo(clampedPoint(event->scenePos()));
        pathItem->setPath(lassoPath);
    } else if (inItemMovement) {
        auto pos = event->scenePos() + selectionPosDelta;
        selectedItem->setPos(pos);
        selectionBox->setPos(pos);
    }
}

void ImageScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (inLassoSelection) {
        lassoPath.closeSubpath();
//        lassoPath = lassoPath.simplified();
        pathItem->setPath(lassoPath.simplified());
        inLassoSelection = false;
        hasLassoSelection = true;
    } else if (inItemMovement) {
        inItemMovement = false;
    }
}

void ImageScene::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (selectedItem != nullptr) {
            removeItem(selectedItem);
            inItemMovement = false;
            pastedPixmaps.removeOne(selectedItem);
            selectedItem = nullptr;
            if (selectionBox != nullptr) {
                removeItem(selectionBox);
                delete selectionBox;
                selectionBox = nullptr;
            }
        } else if (hasLassoSelection) {
            eraseLassoSelection();
        }
    }
}

void ImageScene::drawLineBresenham(utils::Matrix<bool> &mat, QPoint p0, QPoint p1) const {
    int x0 = p0.x(), y0 = p0.y(), x1 = p1.x(), y1 = p1.y();
    int dx = x1 - x0, dy = y1 - y0, inc = 1;
    if (dx == 0 && dy == 0) {
        mat(x0, y0) = true;
    } else if (std::abs(dy) > std::abs(dx)) {
        if (dy < 0) {
            std::swap(x0, x1), std::swap(y0, y1);
            dx = -dx, dy = -dy;
        }
        if (dx < 0) {
            inc = -1;
            dx = abs(dx);
        }
        int x = 0, y = y0;
        float e = -0.5f, k = (float)dx / dy;
        for (int i = 0; i <= dy; ++i) {
            mat(x + x0, y) = true;
            ++y, e += k;
            if (e >= 0.0) x += inc, e -= 1.0;
        }
    } else {
        if (dx < 0) {
            std::swap(x0, x1), std::swap(y0, y1);
            dx = -dx, dy = -dy;
        }
        if (dy < 0) {
            inc = -1;
            dy = abs(dy);
        }
        int x = x0, y = 0;
        float e = -0.5f, k = (float)dy / dx;
        for (int i = 0; i <= dx; ++i) {
            mat(x, y + y0) = true;
            ++x, e += k;
            if (e >= 0.0) y += inc, e -= 1.0;
        }
    }
}

BitMatrix ImageScene::getMaskFromPath(const QPainterPath &path) {
    qDebug() << "ImageScene::getMaskFromPath perf";
    QElapsedTimer timer;
    timer.start();

    auto boundingRect = utils::toAlignedRect(path.boundingRect());
    // Simplified path has already removed all inner crossings
    utils::Matrix<bool> alphaMat(boundingRect.width(), boundingRect.height());
    for (int i = 0; i < path.elementCount(); ++i) {
        QPoint p0 = ((QPointF)path.elementAt(i)).toPoint() - boundingRect.topLeft();
        QPoint p1 = ((QPointF)path.elementAt((i + 1) % path.elementCount())).toPoint() - boundingRect.topLeft();
        drawLineBresenham(alphaMat, p0, p1); // rasterize boundary with Bresenham's algorithm
    }

    qDebug() << "  1. draw line: " << timer.elapsed() << "ms";
    timer.restart();

    // Fill in the inner parts
    const QPoint dir[4] = {{0,  1},
                           {1,  0},
                           {0,  -1},
                           {-1, 0}}; // 4-connected
    auto isValid = [&boundingRect](const QPoint &p) {
        return 0 <= p.x() && p.x() < boundingRect.width() && 0 <= p.y() && p.y() < boundingRect.height();
    };
    utils::Matrix<bool> visited = alphaMat;
    for (int i = 0; i < boundingRect.width(); ++i)
        for (int j = 0; j < boundingRect.height(); ++j) {
            if (visited(i, j)) continue;
            std::vector<QPoint> queue;
            visited(i, j) = true;
            queue.emplace_back(i, j);
            bool isInner = true;
            int head = 0;
            while (head < queue.size()) {
                QPoint p = queue[head++];
                for (auto &d : dir) {
                    QPoint np = p + d;
                    if (!isValid(np)) isInner = false;
                    else if (!visited(np)) {
                        visited(np) = true;
                        queue.push_back(np);
                    }
                }
            }
            // Inner pixels are those that cannot reach the edge of the bounding rect
            if (isInner) {
                for (auto &p : queue)
                    alphaMat(p) = true;
            }
        }

    qDebug() << "  2. floodfill: " << timer.elapsed() << "ms";
    timer.restart();

    BitMatrix ret(alphaMat);
    return ret;
}

QPixmap ImageScene::getSelectedImage() {
    auto *path = getSelection();
    if (path == nullptr) return QPixmap(); // null pixmap (pixmap.isNull() == true)
    if (path == selectionPath) return selectedImage; // using cached image

    auto boundingRect = utils::toAlignedRect(path->boundingRect());
    auto bitMatrix = getMaskFromPath(*path);
    auto mask = QBitmap::fromData(boundingRect.size(), bitMatrix.toBytes(), QImage::Format_MonoLSB);
    selectedImage = pixmap.copy(boundingRect);
    selectedImage.setMask(mask);

    return selectedImage;
}

void ImageScene::eraseLassoSelection() {
    auto *path = getSelection();
    auto boundingRect = utils::toAlignedRect(path->boundingRect());
    auto bitMatrix = getMaskFromPath(*path);
    bgAlpha->pasteSubMatrix(bitMatrix, boundingRect.x(), boundingRect.y());
    auto mask = QBitmap::fromData(imageSize, bgAlpha->toBytes(), QImage::Format_MonoLSB);
//    pixmap.setMask(mask);
    pixmap = QPixmap::fromImage(mask.toImage());
    imageItem->setPixmap(pixmap);
}
