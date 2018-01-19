#include <QtWidgets>

#include "mainwindow.h"
#include "imagewindow.h"

MainWindow::MainWindow()
        : mdiArea(new QMdiArea) {
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(mdiArea);
    connect(mdiArea, &QMdiArea::subWindowActivated,
            this, &MainWindow::updateMenus);

    createActions();
    createStatusBar();
    updateMenus();

    readSettings();

    setWindowTitle(tr("MDI"));
    setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow() != nullptr) {
        event->ignore();
    } else {
        writeSettings();
        event->accept();
    }
}

void MainWindow::newFile() {
    ImageWindow *child = createMdiChild();
//    child->newFile();
    child->show();
}

void MainWindow::open() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QString settingKey = "lastOpenDirectory";
    auto defaultDirectory = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).last();
    auto directory = settings.value(settingKey, defaultDirectory).toString();
    dialog.setDirectory(directory);
    if (dialog.exec() != 0) {
        for (const auto &fileName : dialog.selectedFiles())
            openFile(fileName);
        settings.setValue(settingKey, dialog.directory().absolutePath());
    }
}

bool MainWindow::openFile(const QString &fileName) {
    if (QMdiSubWindow *existing = findMdiChild(fileName)) {
        mdiArea->setActiveSubWindow(existing);
        return true;
    }
    const bool succeeded = loadFile(fileName);
    if (succeeded)
        statusBar()->showMessage(tr("File loaded from %1").arg(fileName), 2000);
    return succeeded;
}

bool MainWindow::loadFile(const QString &fileName) {
    ImageWindow *child = createMdiChild();
    const bool succeeded = child->loadFile(fileName);
    if (succeeded) {
        child->showWithSizeHint(mdiArea->size());
    } else child->close();
    MainWindow::prependToRecentFiles(fileName);
    return succeeded;
}

static inline QString recentFilesKey() { return QStringLiteral("recentFileList"); }

static inline QString fileKey() { return QStringLiteral("file"); }

static QStringList readRecentFiles(QSettings &settings) {
    QStringList result;
    const int count = settings.beginReadArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        result.append(settings.value(fileKey()).toString());
    }
    settings.endArray();
    return result;
}

static void writeRecentFiles(const QStringList &files, QSettings &settings) {
    const int count = files.size();
    settings.beginWriteArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        settings.setValue(fileKey(), files.at(i));
    }
    settings.endArray();
}

bool MainWindow::hasRecentFiles() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const int count = settings.beginReadArray(recentFilesKey());
    settings.endArray();
    return count > 0;
}

void MainWindow::prependToRecentFiles(const QString &fileName) {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    const QStringList oldRecentFiles = readRecentFiles(settings);
    QStringList recentFiles = oldRecentFiles;
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    if (oldRecentFiles != recentFiles)
        writeRecentFiles(recentFiles, settings);

    setRecentFilesVisible(!recentFiles.isEmpty());
}

void MainWindow::setRecentFilesVisible(bool visible) {
    recentFileSubMenuAct->setVisible(visible);
    recentFileSeparator->setVisible(visible);
}

void MainWindow::updateRecentFileActions() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    const QStringList recentFiles = readRecentFiles(settings);
    const int count = qMin(int(MaxRecentFiles), recentFiles.size());
    int i = 0;
    for (; i < count; ++i) {
        const QString fileName = QFileInfo(recentFiles.at(i)).fileName();
        recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
        recentFileActs[i]->setData(recentFiles.at(i));
        recentFileActs[i]->setVisible(true);
    }
    for (; i < MaxRecentFiles; ++i)
        recentFileActs[i]->setVisible(false);
}

void MainWindow::openRecentFile() {
    if (const QAction *action = qobject_cast<const QAction *>(sender()))
        openFile(action->data().toString());
}

void MainWindow::tileWindows() {
    mdiArea->tileSubWindows();
}

void MainWindow::cascadeWindows() {
    mdiArea->cascadeSubWindows();
}

void MainWindow::save() {
//    if (activeMdiChild() && activeMdiChild()->save())
//        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveAs() {
    ImageWindow *child = activeMdiChild();
//    if (child != nullptr && child->saveAs()) {
//        statusBar()->showMessage(tr("File saved"), 2000);
//        MainWindow::prependToRecentFiles(child->currentFile());
//    }
}

void MainWindow::cut() {
    QMessageBox::warning(this, "Not Implemented", "Not Implemented!");
//    if (activeMdiChild())
//        activeMdiChild()->cut();
}

void MainWindow::copy() {
    if (activeMdiChild() != nullptr)
        clipboard = activeMdiChild()->getSelectedImage();
}

void MainWindow::paste() {
    if (activeMdiChild() != nullptr)
        activeMdiChild()->pastePixmap(clipboard);
}

void MainWindow::about() {
    QMessageBox::about(this, tr("About MDI"),
                       tr("The <b>MDI</b> example demonstrates how to write multiple "
                                  "document interface applications using Qt."));
}

void MainWindow::updateMenus() {
    bool hasMdiChild = (activeMdiChild() != nullptr);
    saveAct->setEnabled(hasMdiChild);
    saveAsAct->setEnabled(hasMdiChild);
    pasteAct->setEnabled(hasMdiChild);
    closeAct->setEnabled(hasMdiChild);
    closeAllAct->setEnabled(hasMdiChild);
    tileAct->setEnabled(hasMdiChild);
    cascadeAct->setEnabled(hasMdiChild);
    nextAct->setEnabled(hasMdiChild);
    previousAct->setEnabled(hasMdiChild);
    windowMenuSeparatorAct->setVisible(hasMdiChild);

//    bool hasLassoSelection = (activeMdiChild() && activeMdiChild()->hasLassoSelection());
//    cutAct->setEnabled(hasLassoSelection);
//    copyAct->setEnabled(hasLassoSelection);
    cutAct->setEnabled(hasMdiChild);
    copyAct->setEnabled(hasMdiChild);

//    bool hasPastedPixmaps = (activeMdiChild() && activeMdiChild()->hasPastedPixmaps());
    fusionAct->setEnabled(hasMdiChild);
    smartFillAct->setEnabled(hasMdiChild);
}

void MainWindow::updateWindowMenu() {
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(windowMenuSeparatorAct);

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    windowMenuSeparatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
        QMdiSubWindow *mdiSubWindow = windows.at(i);
        ImageWindow *child = qobject_cast<ImageWindow *>(mdiSubWindow->widget());

        auto text = tr(i < 9 ? "&%1. %2" : "%1. %2").arg(i + 1).arg(child->currentFileName());
        QAction *action = windowMenu->addAction(text, mdiSubWindow, [this, mdiSubWindow]() {
            mdiArea->setActiveSubWindow(mdiSubWindow);
        });
        action->setCheckable(true);
        action->setChecked(child == activeMdiChild());
    }
}

void MainWindow::poissonFusion() {
    if (activeMdiChild() != nullptr)
        activeMdiChild()->poissonFusion();
}

void MainWindow::smartFill() {
    if (activeMdiChild() != nullptr)
        activeMdiChild()->smartFill();
}

ImageWindow *MainWindow::createMdiChild() {
    auto *child = new ImageWindow(this);
    mdiArea->addSubWindow(child);

    return child;
}

void MainWindow::createActions() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));
    fileToolBar->setFloatable(false);

    const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
    newAct = new QAction(newIcon, tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    fileMenu->addAction(newAct);
    fileToolBar->addAction(newAct);

    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAct);
    fileToolBar->addAction(openAct);

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    fileToolBar->addAction(saveAct);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    saveAsAct = new QAction(saveAsIcon, tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);
    fileMenu->addAction(saveAsAct);

    fileMenu->addSeparator();

    QMenu *recentMenu = fileMenu->addMenu(tr("Recent..."));
    connect(recentMenu, &QMenu::aboutToShow, this, &MainWindow::updateRecentFileActions);
    recentFileSubMenuAct = recentMenu->menuAction();

    for (auto &recentFileAct : recentFileActs) {
        recentFileAct = recentMenu->addAction(QString(), this, &MainWindow::openRecentFile);
        recentFileAct->setVisible(false);
    }

    recentFileSeparator = fileMenu->addSeparator();

    setRecentFilesVisible(MainWindow::hasRecentFiles());

    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), qApp, &QApplication::quit);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    fileMenu->addAction(exitAct);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QToolBar *editToolBar = addToolBar(tr("Edit"));
    editToolBar->setFloatable(false);

    const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
    cutAct = new QAction(cutIcon, tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                                    "clipboard"));
    connect(cutAct, &QAction::triggered, this, &MainWindow::cut);
    editMenu->addAction(cutAct);
    editToolBar->addAction(cutAct);

    const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
    copyAct = new QAction(copyIcon, tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                                     "clipboard"));
    connect(copyAct, &QAction::triggered, this, &MainWindow::copy);
    editMenu->addAction(copyAct);
    editToolBar->addAction(copyAct);

    const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
    pasteAct = new QAction(pasteIcon, tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                                      "selection"));
    connect(pasteAct, &QAction::triggered, this, &MainWindow::paste);
    editMenu->addAction(pasteAct);
    editToolBar->addAction(pasteAct);

    QMenu *operationMenu = menuBar()->addMenu(tr("&Operation"));
    QToolBar *operationToolBar = addToolBar(tr("Operation"));
    operationToolBar->setFloatable(false);

    const QIcon fusionIcon = QIcon(":/images/fusion.png");
    fusionAct = new QAction(fusionIcon, tr("F&usion"), this);
    fusionAct->setShortcut({Qt::CTRL + Qt::Key_U});
    fusionAct->setStatusTip("Start Poisson fusion");
    connect(fusionAct, &QAction::triggered, this, &MainWindow::poissonFusion);
    operationMenu->addAction(fusionAct);
    operationToolBar->addAction(fusionAct);

    const QIcon smartFillIcon = QIcon(":/images/smart_fill.png");
    smartFillAct = new QAction(smartFillIcon, tr("Smart &Fill"), this);
    smartFillAct->setShortcut({Qt::CTRL + Qt::Key_F});
    smartFillAct->setStatusTip("Start smart fill");
    connect(smartFillAct, &QAction::triggered, this, &MainWindow::smartFill);
    operationMenu->addAction(smartFillAct);
    operationToolBar->addAction(smartFillAct);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setShortcuts(QKeySequence::Close);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, &QAction::triggered, mdiArea, &QMdiArea::closeActiveSubWindow);

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setShortcuts({QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W)});
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, &QAction::triggered, mdiArea, &QMdiArea::closeAllSubWindows);

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, &QAction::triggered, mdiArea, &QMdiArea::tileSubWindows);

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, &QAction::triggered, mdiArea, &QMdiArea::cascadeSubWindows);

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcuts({QKeySequence(Qt::CTRL + Qt::Key_QuoteLeft), QKeySequence(QKeySequence::NextChild)});
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, &QAction::triggered, mdiArea, &QMdiArea::activateNextSubWindow);

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcuts({QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_QuoteLeft), QKeySequence(QKeySequence::PreviousChild)});
    previousAct->setStatusTip(tr("Move the focus to the previous window"));
    connect(previousAct, &QAction::triggered, mdiArea, &QMdiArea::activatePreviousSubWindow);

    windowMenuSeparatorAct = new QAction(this);
    windowMenuSeparatorAct->setSeparator(true);

    updateWindowMenu();

    menuBar()->addSeparator();

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    aboutAct->setStatusTip(tr("Show the application's About box"));

    QAction *aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

ImageWindow *MainWindow::activeMdiChild() const {
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<ImageWindow *>(activeSubWindow->widget());
    return nullptr;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName) const {
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
    for (auto *window : mdiArea->subWindowList()) {
        ImageWindow *mdiChild = qobject_cast<ImageWindow *>(window->widget());
        if (mdiChild && mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return nullptr;
}
