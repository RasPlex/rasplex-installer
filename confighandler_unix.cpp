#include "confighandler_unix.h"
#include <sys/mount.h>
#include <errno.h>
#include <QDir>
#include <QDebug>

#define RASPLEX_MOUNT_POINT "/tmp/rasplex-mount"

ConfigHandler_unix::~ConfigHandler_unix()
{
    unMount();
}

bool ConfigHandler_unix::implemented()
{
    return false;
}

bool ConfigHandler_unix::mount(const QString &device)
{
    int r = -1;
    QString dev(device);

    if (!QDir().mkpath(RASPLEX_MOUNT_POINT)) {
        qDebug() << "Could not create dir" << RASPLEX_MOUNT_POINT;
        return false;
    }

    // Use first partition
    if (dev.contains("mmcblk")) {
        dev.append("p1");
    }
    else {
        dev.append("1");
    }

    qDebug() << "Mounting" << dev;
#if defined(Q_OS_LINUX)
    r = ::mount(qPrintable(dev), RASPLEX_MOUNT_POINT, "msdos", 0, NULL);
    if (r != 0) {
        qDebug() << "Mount failed:" << QString(strerror(errno));
        return false;
    }
#else
    // OSX mount() call differs from Linux. Todo
    return false;
#endif

    return true;
}


void ConfigHandler_unix::unMount()
{
#if defined(Q_OS_LINUX)
    umount(qPrintable(RASPLEX_MOUNT_POINT)); // this doesn't work on os x
    qDebug() << "Unmounted" << RASPLEX_MOUNT_POINT;
#endif
}
