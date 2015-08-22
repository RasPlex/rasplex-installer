#ifndef DISKWRITER_WINDOWS_H
#define DISKWRITER_WINDOWS_H

#include "diskwriter.h"
#include <QFile>
#include <windows.h>

class DiskWriter_windows : public DiskWriter
{
    Q_OBJECT
public:
    explicit DiskWriter_windows(QObject *parent = 0);
    ~DiskWriter_windows();

private:
    QFile dev;

    bool open(const QString& device);
    void close();
    void sync();
    bool isOpen();
    bool write(const QByteArray &data);

    HANDLE hVolume;
    HANDLE hRawDisk;

    HANDLE getHandleOnDevice(const QString &device, DWORD access) const;
    HANDLE getHandleOnVolume(const QString &volume, DWORD access) const;
    bool getLockOnVolume(HANDLE handle) const;
    bool removeLockOnVolume(HANDLE handle) const;
    bool unmountVolume(HANDLE handle) const;
    bool isVolumeMounted(HANDLE handle) const;
    ULONG deviceNumberFromName(const QString &device) const;
};

#endif // DISKWRITER_WINDOWS_H
