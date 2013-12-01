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

public slots:
    bool writeCompressedImageToRemovableDevice(const QString &filename, const QString &device);

private:
    QFile dev;

    bool open(const QString& device);
    void close();
    void sync();
    bool isOpen();

    HANDLE hVolume;
    HANDLE hRawDisk;

    HANDLE getHandleOnFile(WCHAR *filelocation, DWORD access) const;
    HANDLE getHandleOnDevice(int device, DWORD access) const;
    HANDLE getHandleOnVolume(const QString &volume, DWORD access) const;
    // QString getDriveLabel(const char *drv);
    bool getLockOnVolume(HANDLE handle) const;
    bool removeLockOnVolume(HANDLE handle) const;
    bool unmountVolume(HANDLE handle) const;
    bool isVolumeUnmounted(HANDLE handle) const;
    //char *readSectorDataFromHandle(HANDLE handle, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize);
    bool writeSectorDataToHandle(HANDLE handle, char *data, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize);
    unsigned long long getNumberOfSectors(HANDLE handle, unsigned long long *sectorsize) const;
    //unsigned long long getFileSizeInSectors(HANDLE handle, unsigned long long sectorsize);
    //bool spaceAvailable(char *location, unsigned long long spaceneeded);
    //bool checkDriveType(char *name, ULONG *pid);
    ULONG deviceNumberFromName(const QString &device) const;
};

#endif // DISKWRITER_WINDOWS_H
