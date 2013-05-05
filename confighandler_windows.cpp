#include "confighandler_windows.h"
#include <QDir>
#include <QDebug>

bool ConfigHandler_windows::implemented()
{
    return true;
}

bool ConfigHandler_windows::mount(const QString &device)
{
    // Windows defaults to mounted, but we can at least save the path
    filename = device + "config.txt";
    if (!QDir().exists(filename)) {
        qDebug() << "Could not find config:" << filename;
        return false;
    }
    return true;
}

void ConfigHandler_windows::unMount()
{
    // We don't unmount stuff on windows
}
