#include "diskwriter_unix.h"

#include "zlib.h"

#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QProcess>
#include <blkid/blkid.h>

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

#ifdef Q_OS_MAC
    // Write to RAW device, this is MUCH faster
    device.replace("/dev/","/dev/r");
#endif

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

QStringList DiskWriter_unix::getUserFriendlyNames(QStringList devices) {
    QStringList returnList;

    foreach (QString s, devices) {
#ifdef Q_OS_LINUX
        quint64 size = driveSize(s);
        QStringList partInfo = getPartitionsInfo(s);

        QTextStream friendlyName(&s);
        friendlyName.setRealNumberNotation(QTextStream::FixedNotation);
        friendlyName.setRealNumberPrecision(2);
        friendlyName << " (";
        friendlyName << size/(1024*1024*1024.0) << " GB, partitions: ";
        foreach (QString partition, partInfo) {
            friendlyName << partition << ", ";
        }
        s.chop(2);
        friendlyName << ")";

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

#if defined(Q_OS_LINUX)
quint64 DiskWriter_unix::driveSize(QString device)
{
    blkid_probe pr;

    pr = blkid_new_probe_from_filename(qPrintable(device));
    if (!pr) {
        qDebug() << "Failed to open" << device;
        return -1;
    }

    blkid_loff_t size = blkid_probe_get_size(pr);
    blkid_free_probe(pr);
    return size;
}

QStringList DiskWriter_unix::getPartitionsInfo(QString device)
{
    blkid_probe pr;
    blkid_partlist ls;
    int nparts, i;
    QStringList partList;

    pr = blkid_new_probe_from_filename(qPrintable(device));
    if (!pr) {
        qDebug() << "Failed to open" << device;
        return partList;
    }

    ls = blkid_probe_get_partitions(pr);
    if (ls == NULL) {
        qDebug() << "Failed to get partitions";
        blkid_free_probe(pr);
        return partList;
    }

    nparts = blkid_partlist_numof_partitions(ls);
    if (nparts < 0) {
        qDebug() << "Failed to determine number of partitions";
        blkid_free_probe(pr);
        return partList;
    }

    for (i = 0; i < nparts; i++) {
         blkid_partition par = blkid_partlist_get_partition(ls, i);
         QString partition;
         QTextStream stream(&partition);
         stream << "#" << blkid_partition_get_partno(par) << " - ";
         //stream << " " << blkid_partition_get_start(par);
         stream << " " << blkid_partition_get_size(par)*512/(1024*1024.0) << "MB";
         //stream << " 0x" << hex << blkid_partition_get_type(par);
         //stream << " (" << QString(blkid_partition_get_type_string(par)) << ")";
         //stream << " " << QString(blkid_partition_get_name(par));
         //stream << " " << QString(blkid_partition_get_uuid(par));

         partList << partition;
    }

    blkid_free_probe(pr);
    return partList;

    /*
    if (blkid_do_probe(pr) != 0) {
        qDebug() << "Probing failed on" << device;
        blkid_free_probe(pr);
        continue;
    }

    if (blkid_probe_has_value(pr, "LABEL") == 0) {
        qDebug() << "No label for" << device;
        blkid_free_probe(pr);
        continue;
    }

    if (blkid_probe_lookup_value(pr, "LABEL", &label, NULL) != 0) {
        qDebug() << "Failed to lookup LABEL for" << device;
        blkid_free_probe(pr);
        continue;
    }*/
}
#endif

bool DiskWriter_unix::checkIsMounted(QString device)
{
    char buf[2];
    QFile mountsFile("/proc/mounts");
    if (!mountsFile.open(QFile::ReadOnly)) {
        qDebug() << "Failed to open" << mountsFile.fileName();
        return true;
    }

    // QFile::atEnd() is unreliable for proc
    while (mountsFile.read(buf, 1) > 0) {
        QString line = mountsFile.readLine();
        line.prepend(buf[0]);
        if (line.contains(device)) {
            return true;
        }
    }
    return false;
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
