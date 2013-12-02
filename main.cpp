#include "installer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Installer w;
    w.setWindowTitle("GetRasplex - Offical Rasplex Installer");
    w.show();

    return a.exec();
}
