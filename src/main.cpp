#include <QApplication>
#include <QDir>
#include "lang.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Pomodoro");
    app.setApplicationVersion("1.0.0");
    app.setStyle("Fusion");

    // Initialize locale system (creates data/settings.json)
    QString dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    QDir().mkpath(dataDir);
    LocaleManager::initialize(dataDir);

    MainWindow w;
    w.show();

    int ret = app.exec();
    LocaleManager::destroy();
    return ret;
}
