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

    int open(QString device);
    void close();
    bool isOpen();
    bool writeCompressedImageToRemovableDevice(const QString &filename);
    QStringList getRemovableDeviceNames();
    QStringList getUserFriendlyNames(QStringList devices);

public slots:
    void cancelWrite();

private:
    QFile dev;

    HANDLE hVolume;
    HANDLE hRawDisk;

    HANDLE getHandleOnFile(WCHAR *filelocation, DWORD access);
    HANDLE getHandleOnDevice(int device, DWORD access);
    HANDLE getHandleOnVolume(const QString &volume, DWORD access);
    // QString getDriveLabel(const char *drv);
    bool getLockOnVolume(HANDLE handle);
    bool removeLockOnVolume(HANDLE handle);
    bool unmountVolume(HANDLE handle);
    bool isVolumeUnmounted(HANDLE handle);
    //char *readSectorDataFromHandle(HANDLE handle, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize);
    bool writeSectorDataToHandle(HANDLE handle, char *data, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize);
    unsigned long long getNumberOfSectors(HANDLE handle, unsigned long long *sectorsize);
    //unsigned long long getFileSizeInSectors(HANDLE handle, unsigned long long sectorsize);
    //bool spaceAvailable(char *location, unsigned long long spaceneeded);
    //bool checkDriveType(char *name, ULONG *pid);
    ULONG deviceNumberFromName(const QString &device);
};

#endif // DISKWRITER_WINDOWS_H
