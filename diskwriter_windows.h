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

    static QString errorAsString(DWORD error);

private:
    friend class DeviceEnumerator_windows;
    QFile dev;

    bool open(const QString& device);
    void close();
    void sync();
    bool isOpen();
    bool write(const QByteArray &data);

    HANDLE hVolume;
    HANDLE hRawDisk;

    static HANDLE getHandleOnDevice(const QString &device, DWORD access);
    static HANDLE getHandleOnVolume(const QString &volume, DWORD access);
    bool getLockOnVolume(HANDLE handle) const;
    bool removeLockOnVolume(HANDLE handle) const;
    bool unmountVolume(HANDLE handle) const;
    bool isVolumeMounted(HANDLE handle) const;
    static ULONG deviceNumberFromName(const QString &device);
};

#endif // DISKWRITER_WINDOWS_H
