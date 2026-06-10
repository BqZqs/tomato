#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Pomodoro");
    app.setApplicationVersion("1.0.0");
    app.setStyle("Fusion");

    MainWindow w;
    w.show();
    return app.exec();
}
