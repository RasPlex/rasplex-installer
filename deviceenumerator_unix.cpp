#include "deviceenumerator_unix.h"

#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QProcess>
#if defined(Q_OS_LINUX)
#include <blkid/blkid.h>
#include <unistd.h>
#endif

#define USE_ONLY_USB_DEVICES_ON_OSX 1
QStringList DeviceEnumerator_unix::getRemovableDeviceNames() const
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

QStringList DeviceEnumerator_unix::getUserFriendlyNames(const QStringList &devices) const
{
    QStringList returnList;

    foreach (QString s, devices) {
#ifdef Q_OS_LINUX
        qint64 size = driveSize(s);
        QStringList partInfo = getPartitionsInfo(s);

        QTextStream friendlyName(&s);
        friendlyName.setRealNumberNotation(QTextStream::FixedNotation);
        friendlyName.setRealNumberPrecision(2);
        friendlyName << " (";
        if (size > 0) {
            friendlyName << size/(1024*1024*1024.0) << " GB";
        }
        else {
            friendlyName << "??? GB";
        }
        friendlyName << ", partitions: ";
        foreach (QString partition, partInfo) {
            friendlyName << partition << ", ";
        }
        s.chop(2);
        friendlyName << ")";

        returnList.append(s);
#else
        QProcess lsblk;
        lsblk.start(QString("diskutil info %1").arg(s), QIODevice::ReadOnly);
        lsblk.waitForStarted();
        lsblk.waitForFinished();

        QString output = lsblk.readLine();
        QStringList iddata;
        QString item = "";
        while (!lsblk.atEnd()) {
            output = output.trimmed(); // Odd trailing whitespace
            if (output.contains("Device / Media Name:")) { // We want the volume name of this device
                output.replace("Device / Media Name:      ","");
                iddata.append(output);
            }else if (output.contains("Device Identifier:")) { // We want the volume name of this device
                output.replace("Device Identifier:        ","");
                iddata.append(output);
            }else if (output.contains("Total Size:")) { // We want the volume name of this device
                 output.replace("Total Size:               ","");
                 QStringList tokens = output.split(" ");
                 iddata.append( "("+tokens[0]+tokens[1]+")");
            }

            output = lsblk.readLine();
        }

        foreach(QString each,iddata)
        {
            item += each+": ";
        }

        returnList.append(item);
#endif
    }

    return returnList;
}

bool DeviceEnumerator_unix::checkIsMounted(const QString &device) const
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

bool DeviceEnumerator_unix::checkIfUSB(const QString &device) const
{
#ifdef Q_OS_LINUX
    QString path = "/sys/block/" + device;
    QByteArray devPath(256, '\0');
    ssize_t rc = readlink(path.toLocal8Bit().data(), devPath.data(), devPath.size());
    if (rc && devPath.contains("usb")) {
        return true;
    }
    return false;
#else
    QProcess lssize;
    lssize.start(QString("diskutil info %1").arg(device), QIODevice::ReadOnly);
    lssize.waitForStarted();
    lssize.waitForFinished();

    QString s = lssize.readLine();
    while (!lssize.atEnd()) {
         if (s.contains("Protocol:") && ( s.contains("USB") || s.contains("Secure Digital"))) {
             return true;
         }
         s = lssize.readLine();
    }

    return false;
#endif
}

QStringList DeviceEnumerator_unix::getDeviceNamesFromSysfs() const
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

        if (device.startsWith("mmcblk")) {
            names << device;
        }
        else if (device.startsWith("sd") && checkIfUSB(device)) {
            names << device;
        }
    }

    return names;
}

#if defined(Q_OS_LINUX)
qint64 DeviceEnumerator_unix::driveSize(const QString& device) const
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

QStringList DeviceEnumerator_unix::getPartitionsInfo(const QString& device) const
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
