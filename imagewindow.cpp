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
