#ifndef DISKWRITER_UNIX_H
#define DISKWRITER_UNIX_H

#include "diskwriter.h"
#include <QFile>

class DiskWriter_unix : public DiskWriter
{
    Q_OBJECT
public:
    explicit DiskWriter_unix(QObject *parent = 0);
    ~DiskWriter_unix();

    int open(QString device);
    void close();
    bool isOpen();
    bool writeCompressedImageToRemovableDevice(const QString &filename);
    QStringList getRemovableDeviceNames();
    void cancelWrite();
    QStringList getUserFriendlyNamesRemovableDevices(QStringList devices);

private:
    QFile dev;

    bool checkIsMounted(QString device);
    QStringList getDeviceNamesFromSysfs();
};

#endif // DISKWRITER_UNIX_H
