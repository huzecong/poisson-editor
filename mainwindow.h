#ifndef POISSONEDITOR_MAINWINDOW_H
#define POISSONEDITOR_MAINWINDOW_H

#include <QMainWindow>

class ImageWindow;

class QAction;

class QMenu;

class QMdiArea;

class QMdiSubWindow;

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    MainWindow();

    bool openFile(const QString &fileName);
    void tileWindows();
    void cascadeWindows();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void updateRecentFileActions();
    void openRecentFile();
    void cut();
    void copy();
    void paste();
    void about();
    void updateMenus();
    void updateWindowMenu();
    ImageWindow *createMdiChild();

private:
    enum { MaxRecentFiles = 5 };

    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool loadFile(const QString &fileName);
    static bool hasRecentFiles();
    void prependToRecentFiles(const QString &fileName);
    void setRecentFilesVisible(bool visible);
    ImageWindow *activeMdiChild() const;
    QMdiSubWindow *findMdiChild(const QString &fileName) const;

    void poissonFusion();
    void smartFill();

    QMdiArea *mdiArea;

    QPixmap clipboard;

    QMenu *windowMenu;
    QAction *newAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *recentFileActs[MaxRecentFiles];
    QAction *recentFileSeparator;
    QAction *recentFileSubMenuAct;

    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;

    QAction *fusionAct;
    QAction *smartFillAct;

    QAction *closeAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *windowMenuSeparatorAct;
};

#endif //POISSONEDITOR_MAINWINDOW_H
