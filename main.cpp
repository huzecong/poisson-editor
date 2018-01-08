#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(graphics);

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Poisson Image Editing");
    QCoreApplication::setOrganizationName("Zecong Hu");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Poisson Image Editing");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(app);

    MainWindow mainWindow;
    for (const auto &fileName : parser.positionalArguments())
        mainWindow.openFile(fileName);
    mainWindow.show();
    return app.exec();
}
