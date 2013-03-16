#include "installer.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Installer w;
    w.show();
    
    return a.exec();
}
