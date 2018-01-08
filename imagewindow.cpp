#include <QtWidgets>
#include <queue>

#if defined(QT_PRINTSUPPORT_LIB)
//#if QT_CONFIG(printdialog)
//#endif
#endif

#include "imagewindow.h"
#include "utils.h"

ImageScene::ImageScene() {
    pathItem = new QGraphicsPathItem;
    pathItem->setPen(QPen(Qt::black, 2, Qt::DashLine));
    pathItem->setBrush(QBrush(QColor(0, 100, 200, 50))); // half-transparent light blue
    pathItem->setZValue(100); // put on top of everything else
    addItem(pathItem);
}

void ImageScene::setPixmap(const QPixmap &pixmap) {
    if (imageItem != nullptr)
        removeItem(imageItem);

    imageSize = pixmap.size();
    imageItem = new QGraphicsPixmapItem(pixmap);
    imageItem->setZValue(0);
    imageItem->setTransformationMode(Qt::SmoothTransformation);
    addItem(imageItem);
}

const QPainterPath *ImageScene::getSelection() const {
    return hasSelection ? &lassoPath : nullptr;
}

QPointF ImageScene::clampedPoint(const QPointF &point) {
    auto pos = point;
    pos.setX(utils::clamp(pos.x(), 0.0, static_cast<qreal>(imageSize.width())));
    pos.setY(utils::clamp(pos.y(), 0.0, static_cast<qreal>(imageSize.height())));
    return pos;
}

void ImageScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton && imageItem != nullptr) {
        inSelection = true;
        lassoPath = QPainterPath(clampedPoint(event->scenePos()));
        lassoPath.setFillRule(Qt::WindingFill);
        pathItem->setPath(lassoPath);
    }
}

void ImageScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (inSelection) {
        lassoPath.lineTo(clampedPoint(event->scenePos()));
        pathItem->setPath(lassoPath);
    }
}

void ImageScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (inSelection) {
        lassoPath.closeSubpath();
        lassoPath = lassoPath.simplified();
        pathItem->setPath(lassoPath);
        inSelection = false;
        hasSelection = true;
    }
}

ImageWindow::ImageWindow(QWidget *parent) : QMainWindow(parent) {
    scene = new ImageScene;
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);

    setCentralWidget(view);
    view->setMouseTracking(true);

    setStatusBar(new QStatusBar);
    zoomSlider = new QSlider(Qt::Horizontal, statusBar());
    zoomSlider->setFixedWidth(200);
    zoomSlider->setFocusPolicy(Qt::NoFocus);
    zoomSlider->setRange(1, 200);
    zoomSlider->setTickPosition(QSlider::TicksBothSides);
    zoomSlider->setTickInterval(80);
    zoomSlider->setValue(100);
    statusBar()->addWidget(zoomSlider);
    zoomScaleLabel = new QLabel;
    statusBar()->addWidget(zoomScaleLabel);
    connect(zoomSlider, &QSlider::sliderMoved, [&](int value) {
        double actualScale = value <= 100
                             ? value / 100.0
                             : (value - 100) / 25.0 + 1;
        this->setSlider(actualScale);
        view->resetMatrix();
        view->scale(scale, scale);
    });
}

void ImageWindow::setSlider(double scale) {
    scale = utils::clamp(scale, 0.01, 5.0);
    this->scale = scale;
    auto percentage = static_cast<int>(scale * 100);
    zoomScaleLabel->setText(QStringLiteral("%1%").arg(percentage));

    int sliderValue = percentage <= 100 ? percentage : 100 + (percentage - 100) / 4;
    if (zoomSlider->value() != sliderValue)
        zoomSlider->setValue(sliderValue);

    if (view->matrix().m11() != scale) {
        view->resetMatrix();
        view->scale(scale, scale);
    }
}

bool ImageWindow::loadFile(const QString &filePath) {
    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2").arg(QDir::toNativeSeparators(filePath), reader.errorString()));
        return false;
    }

    originalImage = QPixmap::fromImage(image);
    scene->setPixmap(originalImage);
    imageSize = image.size();

    setWindowFilePath(QFileInfo(filePath).canonicalFilePath());

    return true;
}

bool ImageWindow::event(QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (dynamic_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
            auto *image = getSelectedImage();
            scene->addItem(new QGraphicsPixmapItem(*image));
        }
    }
    if (event->type() == QEvent::Gesture)
        return gestureEvent(dynamic_cast<QGestureEvent *>(event));
    if (event->type() == QEvent::NativeGesture)
        return nativeGestureEvent(dynamic_cast<QNativeGestureEvent *>(event));
    inGesture = false;
    return QMainWindow::event(event);
}

bool ImageWindow::gestureEvent(QGestureEvent *event) {
    if (QGesture *gesture = event->gesture(Qt::PinchGesture)) {
        auto *pinch = dynamic_cast<QPinchGesture *>(gesture);
        qDebug() << pinch->centerPoint() << pinch->hotSpot() << pinch->scaleFactor();
    } else {
        inGesture = false;
    }
    return true;
}

bool ImageWindow::nativeGestureEvent(QNativeGestureEvent *event) {
    if (event->gestureType() == Qt::ZoomNativeGesture) {
        if (!inGesture) {
            inGesture = true;
            scaleBeforeGesture = scale;
            cumulativeScale = 0.0;
        }
        cumulativeScale += event->value();
        auto pos = event->localPos();
        auto targetScenePos = view->mapToScene(event->pos());
        double newScale = scaleBeforeGesture * (1.0 + cumulativeScale);
        setSlider(newScale);
        if (scale == newScale) {  // scale is not clamped
            view->resetTransform();
            view->scale(scale, scale);
            view->centerOn(targetScenePos);
            auto deltaViewportPos = event->pos() - QPointF(view->viewport()->width() / 2.0, view->viewport()->height() / 2.0);
            auto viewportCenter = view->mapFromScene(targetScenePos) - deltaViewportPos;
            view->centerOn(view->mapToScene(viewportCenter.toPoint()));  // still buggy, but fuck it
        }
    } else {
        inGesture = false;
    }
    return true;
}

const QString ImageWindow::currentFile() const {
    return windowFilePath();
}

const QString ImageWindow::currentFileName() const {
    return QFileInfo(windowFilePath()).fileName();
}

void ImageWindow::showWithSizeHint(QSize parentSize) {
    if (imageSize.height() > parentSize.height() || imageSize.width() > parentSize.width()) {
        view->fitInView(QRect(QPoint(0, 0), imageSize), Qt::KeepAspectRatio);
        setSlider(view->matrix().m11());
        showMaximized();
    } else {
        resize(imageSize);
        show();
    }
}

const bool ImageWindow::hasSelection() const {
    return scene->getSelection() != nullptr;
}

void ImageWindow::drawLineBresenham(utils::BitMatrix &mat, QPoint p0, QPoint p1) const {
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

const QPixmap * ImageWindow::getSelectedImage() {
    auto *path = scene->getSelection();
    if (path == nullptr) return nullptr;
    if (path == selectionPath) return &selectedImage; // using cached image

    // minimum containing integer bounding rect
    QPoint topLeft = QPoint(qFloor(path->boundingRect().x()), qFloor(path->boundingRect().y()));
    QPoint bottomRight = QPoint(qCeil(path->boundingRect().right()), qCeil(path->boundingRect().bottom()));
    QRect boundingRect = QRect(topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x() + 1, bottomRight.y() - topLeft.y() + 1);
    // simplified path has already removed all inner crossings
    utils::BitMatrix alphaMat(boundingRect.width(), boundingRect.height());
    for (int i = 0; i < path->elementCount(); ++i) {
        QPoint p0 = ((QPointF)path->elementAt(i)).toPoint() - boundingRect.topLeft();
        QPoint p1 = ((QPointF)path->elementAt((i + 1) % path->elementCount())).toPoint() - boundingRect.topLeft();
        drawLineBresenham(alphaMat, p0, p1); // rasterize boundary with Bresenham's algorithm
    }

    auto image = QImage(boundingRect.size(), QImage::Format_RGB32);
    for (int i = 0; i < boundingRect.width(); ++i)
        for (int j = 0; j < boundingRect.height(); ++j)
            image.setPixelColor(i, j, alphaMat(i, j) ? Qt::red : Qt::black);

    // fill in the inner parts
    const QPoint dir[4] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}}; // 4-connected
    auto isValid = [&](const QPoint &p) {
        return 0 <= p.x() && p.x() < boundingRect.width() && 0 <= p.y() && p.y() < boundingRect.height();
    };
    utils::BitMatrix visited = alphaMat;
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
            // inner pixels are those that cannot reach the edge of the bounding rect
            if (isInner) {
                for (auto &p : queue)
                    alphaMat(p) = true;
            }
        }

//    for (int i = 0; i < boundingRect.width(); ++i)
//        for (int j = 0; j < boundingRect.height(); ++j)
//            if (image.pixelColor(i, j) == Qt::black)
//                image.setPixelColor(i, j, alphaMat(i, j) ? Qt::white : Qt::black);

    selectedImage = originalImage.copy(boundingRect);
    auto mask = QBitmap::fromData(boundingRect.size(), alphaMat.toBytes(), QImage::Format_MonoLSB);
//    selectedImage = QPixmap::fromImage(mask.toImage());
    selectedImage.setMask(mask);
//    selectedImage = QPixmap::fromImage(image);

    return &selectedImage;
}

/*
bool ImageWindow::saveFile(const QString &fileName) {
    QImageWriter writer(fileName);

    if (!writer.write(image)) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot write %1: %2")
                                         .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    statusBar()->showMessage(message);
    return true;
}


static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode) {
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
                                              ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
            foreach(
            const QByteArray &mimeTypeName, supportedMimeTypes)mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

bool ImageWindow::open() {
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    return dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first());
}

bool ImageWindow::saveAs() {
    QFileDialog dialog(this, tr("Save File As"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

    return dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first());
}

void ImageWindow::copy() {
#ifndef QT_NO_CLIPBOARD
    QGuiApplication::clipboard()->setImage(image);
#endif // !QT_NO_CLIPBOARD
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull())
                return image;
        }
    }
    return QImage();
}
#endif // !QT_NO_CLIPBOARD

void ImageWindow::paste() {
#ifndef QT_NO_CLIPBOARD
    const QImage newImage = clipboardImage();
    if (newImage.isNull()) {
        statusBar()->showMessage(tr("No image in clipboard"));
    } else {
        setImage(newImage);
        setWindowFilePath(QString());
        const QString message = tr("Obtained image from clipboard, %1x%2, Depth: %3")
                .arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        statusBar()->showMessage(message);
    }
#endif // !QT_NO_CLIPBOARD
}

void ImageWindow::zoomIn() {
    scaleImage(1.25);
}

void ImageWindow::zoomOut() {
    scaleImage(0.8);
}

void ImageWindow::normalSize() {
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}

void ImageWindow::fitToWindow() {
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow)
        normalSize();
    updateActions();
}

void ImageWindow::createActions() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &ImageWindow::open);
    openAct->setShortcut(QKeySequence::Open);

    saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &ImageWindow::saveAs);
    saveAsAct->setEnabled(false);

    printAct = fileMenu->addAction(tr("&Print..."), this, &ImageWindow::print);
    printAct->setShortcut(QKeySequence::Print);
    printAct->setEnabled(false);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcut(tr("Ctrl+Q"));

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    copyAct = editMenu->addAction(tr("&Copy"), this, &ImageWindow::copy);
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);

    QAction *pasteAct = editMenu->addAction(tr("&Paste"), this, &ImageWindow::paste);
    pasteAct->setShortcut(QKeySequence::Paste);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &ImageWindow::zoomIn);
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);

    zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &ImageWindow::zoomOut);
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);

    normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &ImageWindow::normalSize);
    normalSizeAct->setShortcut(tr("Ctrl+S"));
    normalSizeAct->setEnabled(false);

    viewMenu->addSeparator();

    fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &ImageWindow::fitToWindow);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    helpMenu->addAction(tr("&About"), this, &ImageWindow::about);
    helpMenu->addAction(tr("About &Qt"), &QApplication::aboutQt);
}

void ImageWindow::updateActions() {
    saveAsAct->setEnabled(!image.isNull());
    copyAct->setEnabled(!image.isNull());
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void ImageWindow::scaleImage(double factor) {
    Q_ASSERT(imageLabel->pixmap());
    scaleFactor *= factor;
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void ImageWindow::adjustScrollBar(QScrollBar *scrollBar, double factor) {
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep() / 2)));
}
*/
