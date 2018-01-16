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
    parser.addOption({"tile", "Tile windows."});
    parser.addOption({"cascade", "Cascade windows."});
    parser.process(app);

    if (parser.isSet("tile") && parser.isSet("cascade"))
        throw std::runtime_error("Cannot set both tile and cascade flags");

    MainWindow mainWindow;
    for (const auto &fileName : parser.positionalArguments())
        mainWindow.openFile(fileName);
    mainWindow.show();
    if (parser.isSet("tile")) mainWindow.tileWindows();
    else if (parser.isSet("cascade")) mainWindow.cascadeWindows();
    return app.exec();
}
