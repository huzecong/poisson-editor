#include <queue>

#include "imagewindow.h"


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

    setSlider(1.0);
}

ImageWindow::~ImageWindow() {
    delete view;
    delete scene;
    delete zoomSlider;
    delete zoomScaleLabel;
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
        /*
        if (dynamic_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
            auto *image = getSelectedImage();
            scene->addItem(new QGraphicsPixmapItem(*image));
        }
         */
    }
//    if (event->type() == QEvent::Gesture)
//        return gestureEvent(dynamic_cast<QGestureEvent *>(event));
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
        auto pos = event->pos();
        auto targetScenePos = view->mapToScene(pos);
        double newScale = scaleBeforeGesture * (1.0 + cumulativeScale);
        setSlider(newScale);
        if (scale == newScale) {  // scale is not clamped
            view->resetTransform();
            view->scale(scale, scale);
            view->centerOn(targetScenePos);
            auto deltaViewportPos = pos - QPointF(view->viewport()->width() / 2.0, view->viewport()->height() / 2.0);
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

const bool ImageWindow::hasPastedPixmaps() const {
    return scene->getPastedPixmaps().length() > 0;
}

QPixmap ImageWindow::getSelectedImage() {
    return scene->getSelectedImage();
}

void ImageWindow::pastePixmap(const QPixmap &pixmap) {
    scene->pastePixmap(pixmap);
}

void ImageWindow::poissonFusion() {
    scene->poissonFusion();
}

void ImageWindow::smartFill() {
    scene->smartFill();
}

bool ImageWindow::saveFile() {
    QImageWriter writer(windowFilePath());

    if (!writer.write(scene->getImage())) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot write %1: %2").arg(QDir::toNativeSeparators(windowFilePath())),
                                 writer.errorString());
        return false;
    }
    return true;
}
