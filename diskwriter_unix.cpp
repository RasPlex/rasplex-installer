#include "diskwriter_unix.h"

#include "zlib.h"

#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QProcess>

DiskWriter_unix::DiskWriter_unix(QObject *parent) :
    DiskWriter(parent)
{
    isCancelled = false;
}

DiskWriter_unix::~DiskWriter_unix()
{
    if (dev.isOpen()) {
        dev.close();
    }
}

int DiskWriter_unix::open(QString device)
{
    dev.setFileName(device);

#ifdef Q_OS_MAC
    QProcess unmount;
    unmount.start("diskutil unmountDisk "+device, QIODevice::ReadOnly);
    unmount.waitForStarted();
    unmount.waitForFinished();
    qDebug() << unmount.readAll();
#endif

    if (!dev.open(QFile::WriteOnly)) {
        return -1;
    }
    return 0;
}

QStringList DiskWriter_unix::getUserFriendlyNamesRemovableDevices(QStringList devices) {
    QStringList returnList;

    foreach (QString s, devices) {
#ifdef Q_OS_LINUX
        // TODO
        returnList.append(s);
#else
        QProcess lsblk;
        lsblk.start(QString("diskutil info %1s1").arg(s), QIODevice::ReadOnly);
        lsblk.waitForStarted();
        lsblk.waitForFinished();

        QString output = lsblk.readLine();
        while (!lsblk.atEnd()) {
            output = output.trimmed(); // Odd trailing whitespace
            if (output.contains("Volume Name:")) { // We want the volume name of this device
                output.replace("Volume Name:              ","");
                returnList.append(output);
            }
            output = lsblk.readLine();
        }
#endif
    }

    return returnList;
}

void DiskWriter_unix::close()
{
    dev.close();
}

bool DiskWriter_unix::isOpen()
{
    return dev.isOpen();
}

void DiskWriter_unix::cancelWrite()
{
    isCancelled = true;
}

bool DiskWriter_unix::writeCompressedImageToRemovableDevice(const QString &filename)
{
    int r;
    bool ok;
    // 512 == common sector size
    char buf[512*1024];

    if (!dev.isOpen()) {
        qDebug() << "Device not ready or whatever";
        return false;
    }

    // Open source
    gzFile src = gzopen(filename.toStdString().c_str(), "rb");
    if (src == NULL) {
        qDebug() << "Couldn't open file:" << filename;
        return false;
    }

    if (gzbuffer(src, 128*1024) != 0) {
        qDebug() << "Failed to set buffer size";
        gzclose_r(src);
        return false;
    }

    r = gzread(src, buf, sizeof(buf));
    while (r > 0 && ! isCancelled) {
        // TODO: Sanity check
        ok = dev.write(buf, r);
        if (!ok) {
            qDebug() << "Error writing";
            return false;
        }
        emit bytesWritten(gztell(src));
        QApplication::processEvents();
        r = gzread(src, buf, sizeof(buf));
    }

    isCancelled = false;
    if (r < 0) {
        gzclose_r(src);
        qDebug() << "Error reading file!";
        return false;
    }

    dev.close();
    gzclose_r(src);
    return true;
}

#define USE_ONLY_USB_DEVICES_ON_OSX 1
QStringList DiskWriter_unix::getRemovableDeviceNames()
{

    QStringList names;
    QStringList unmounted;

#ifdef Q_OS_LINUX
    names = getDeviceNamesFromSysfs();

    foreach (QString device, names)
    {
        if (! checkIsMounted(device))
            unmounted << "/dev/"+device;
    }

    return unmounted;
#else
    QProcess lsblk;
    lsblk.start("diskutil list", QIODevice::ReadOnly);
    lsblk.waitForStarted();
    lsblk.waitForFinished();

    QString device = lsblk.readLine();
    while (!lsblk.atEnd()) {
        device = device.trimmed(); // Odd trailing whitespace

        if (device.startsWith("/dev/disk")) {
            QString name = device.split(QRegExp("\\s+")).first();
#if USE_ONLY_USB_DEVICES_ON_OSX == 0
	    names << name;
#else
            // We only want to add USB devics
            if (this->checkIfUSB(name)) {
                names << name;
            }
#endif
        }
        device = lsblk.readLine();
    }

    return names;
#endif
}

bool DiskWriter_unix::checkIfUSB(QString device) {
#ifdef Q_OS_LINUX
    //TODO
    return true;
#else
    QProcess lssize;
    lssize.start(QString("diskutil info %1").arg(device), QIODevice::ReadOnly);
    lssize.waitForStarted();
    lssize.waitForFinished();

    QString s = lssize.readLine();
    while (!lssize.atEnd()) {
         if (s.contains("Protocol:") && s.contains("USB")) {
             return true;
         }
         s = lssize.readLine();
    }

    return false;
#endif
}


bool DiskWriter_unix::checkIsMounted(QString device)
{
    bool mounted = false;
    QProcess chkmount;
    chkmount.start("mount",QIODevice::ReadOnly);
    chkmount.waitForStarted();
    chkmount.waitForFinished();

    QString mount = chkmount.readLine();
    while (!chkmount.atEnd() && ! mounted) {
        mount = mount.trimmed(); // Odd trailing whitespace
        mount = mount.split(QRegExp("\\s+")).first();
        mounted = mount.indexOf(device) >= 0;
        mount = chkmount.readLine();
    }

    return mounted;
}

QStringList DiskWriter_unix::getDeviceNamesFromSysfs()
{
    QStringList names;

    QDir currentDir("/sys/block");
    currentDir.setFilter(QDir::Dirs);

    QStringList entries = currentDir.entryList();
    foreach (QString device, entries) {
        // Skip "." and ".." dir entries
        if (device == "." || device == "..") {
            continue;
        }

        if (device.startsWith("mmcblk") || device.startsWith("sd")) {
            names << device;
        }
    }

    return names;
}
