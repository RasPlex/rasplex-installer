#include "diskwriter_windows.h"

#include "zlib.h"

#include <QDebug>
#include <QApplication>
#include <QMessageBox>

DiskWriter_windows::DiskWriter_windows(QObject *parent) :
    DiskWriter(parent),
    hVolume(INVALID_HANDLE_VALUE),
    hRawDisk(INVALID_HANDLE_VALUE)
{
    isCancelled = false;
}

DiskWriter_windows::~DiskWriter_windows()
{
    if (isOpen()) {
        close();
    }
}

bool DiskWriter_windows::open(const QString &device)
{
    QString devString = device;
    if (devString.endsWith('\\')) {
        devString.chop(1);
    }
    int deviceID = deviceNumberFromName(devString);

    hVolume = getHandleOnVolume(devString, GENERIC_WRITE);
    if (hVolume == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (!getLockOnVolume(hVolume)) {
        close();
        return false;
    }

    if (!unmountVolume(hVolume)) {
        close();
        return false;
    }

    hRawDisk = getHandleOnDevice(deviceID, GENERIC_WRITE);
    if (hRawDisk == INVALID_HANDLE_VALUE) {
        close();
        return false;
    }

    return true;
}

void DiskWriter_windows::close()
{
    if (hRawDisk != INVALID_HANDLE_VALUE) {
        CloseHandle(hRawDisk);
        hRawDisk = INVALID_HANDLE_VALUE;
    }
    if (hVolume != INVALID_HANDLE_VALUE) {
        removeLockOnVolume(hVolume);
        CloseHandle(hVolume);
        hVolume = INVALID_HANDLE_VALUE;
    }
}

void DiskWriter_windows::sync()
{
    FlushFileBuffers(hVolume);
}

bool DiskWriter_windows::isOpen()
{
    return (hRawDisk != INVALID_HANDLE_VALUE && hVolume != INVALID_HANDLE_VALUE);
}

void DiskWriter_windows::cancelWrite()
{
    isCancelled = true;
}

bool DiskWriter_windows::writeCompressedImageToRemovableDevice(const QString &filename, const QString &device)
{
    int r;
    bool ok;
    // 512 == common sector size. TODO: read sector size
    char buf[512*1024];
    isCancelled = false;

    if (!open(device)) {
        emit error("Couldn't open " + device);
        return false;
    }

    // Open source
    gzFile src = gzopen(filename.toStdString().c_str(), "rb");
    if (src == NULL) {
        emit error("Couldn't open " + filename);
        this->close();
        return false;
    }

    if (gzbuffer(src, 128*1024) != 0) {
        emit error("Failed to set gz buffer size");
        gzclose_r(src);
        this->close();
        return false;
    }

    DWORD byteswritten;
    r = gzread(src, buf, sizeof(buf));
    while (r > 0 && ! isCancelled) {
        ok = WriteFile(hRawDisk, buf, r, &byteswritten, NULL);
        if (!ok) {
            wchar_t *errormessage=NULL;
            FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
            QString errText = QString::fromUtf16((const ushort *)errormessage);
            QMessageBox::critical(NULL, QObject::tr("Write Error"),
                                  QObject::tr("An error occurred when attempting to write data to handle.\n"
                                              "Error %1: %2").arg(GetLastError()).arg(errText));
            LocalFree(errormessage);
            emit error("Failed to write to " + device + "!");
            gzclose(src);
            this->close();
            return false;
        }
        emit bytesWritten(gztell(src));
        // Needed for cancelWrite() to work
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        r = gzread(src, buf, sizeof(buf));
    }

    if (r < 0) {
        emit error("Failed to read from " + filename + "!");
        gzclose_r(src);
        this->close();
        return false;
    }

    emit syncing();
    gzclose_r(src);
    this->sync();
    this->close();

    if (!isCancelled) {
        emit finished();
    }
    isCancelled = false;
    return true;
}

// Adapted from win32 DiskImager
HANDLE DiskWriter_windows::getHandleOnFile(WCHAR *filelocation, DWORD access)
{
    HANDLE hFile;
    hFile = CreateFile(filelocation, access, 0, NULL, (access == GENERIC_READ) ? OPEN_EXISTING:CREATE_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        wchar_t *errormessage=NULL;
        ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0,
                         (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("File Error"), QObject::tr("An error occurred when attempting to get a handle on the file.\n"
                                                              "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
    }
    return hFile;
}

// Adapted from win32 DiskImager
HANDLE DiskWriter_windows::getHandleOnDevice(int device, DWORD access)
{
    HANDLE hDevice;
    QString devicename = QString("\\\\.\\PhysicalDrive%1").arg(device);
    qDebug() << devicename;
    hDevice = CreateFile(devicename.toStdWString().c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Device Error"),
                              QObject::tr("An error occurred when attempting to get a handle on the device.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
    }
    return hDevice;
}

// Adapted from win32 DiskImager
HANDLE DiskWriter_windows::getHandleOnVolume(const QString &volume, DWORD access)
{
    HANDLE hVolume;
    QString volumename = "\\\\.\\" + volume;
    qDebug() << volumename;
    hVolume = CreateFile(volumename.toStdWString().c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Volume Error"),
                              QObject::tr("An error occurred when attempting to get a handle on the volume.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
    }
    return hVolume;
}

// Adapted from win32 DiskImager
bool DiskWriter_windows::getLockOnVolume(HANDLE handle)
{
    DWORD bytesreturned;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesreturned, NULL);
    if (!bResult)
    {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Lock Error"),
                              QObject::tr("An error occurred when attempting to lock the volume.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
    }
    return (bResult);
}

// Adapted from win32 DiskImager
bool DiskWriter_windows::removeLockOnVolume(HANDLE handle)
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &junk, NULL);
    if (!bResult)
    {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Unlock Error"),
                              QObject::tr("An error occurred when attempting to unlock the volume.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
    }
    return (bResult);
}

// Adapted from win32 DiskImager
bool DiskWriter_windows::unmountVolume(HANDLE handle)
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &junk, NULL);
    if (!bResult)
    {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Dismount Error"),
                              QObject::tr("An error occurred when attempting to dismount the volume.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
    }
    return (bResult);
}

// Adapted from win32 DiskImager
bool DiskWriter_windows::isVolumeUnmounted(HANDLE handle)
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &junk, NULL);
    return (!bResult);
}

// Adapted from win32 DiskImager
bool DiskWriter_windows::writeSectorDataToHandle(HANDLE handle, char *data, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize)
{
    unsigned long byteswritten;
    BOOL bResult;
    LARGE_INTEGER li;
    li.QuadPart = startsector * sectorsize;
    SetFilePointer(handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    bResult = WriteFile(handle, data, sectorsize * numsectors, &byteswritten, NULL);
    if (!bResult)
    {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Write Error"),
                              QObject::tr("An error occurred when attempting to write data to handle.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
    }
    return (bResult);
}

// Adapted from win32 DiskImager
unsigned long long DiskWriter_windows::getNumberOfSectors(HANDLE handle, unsigned long long *sectorsize)
{
    DWORD junk;
    DISK_GEOMETRY_EX diskgeometry;
    BOOL bResult;
    bResult = DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskgeometry, sizeof(diskgeometry), &junk, NULL);
    if (!bResult)
    {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Device Error"),
                              QObject::tr("An error occurred when attempting to get the device's geometry.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
        return 0;
    }
    if (sectorsize != NULL)
    {
        *sectorsize = (unsigned long long)diskgeometry.Geometry.BytesPerSector;
    }
    return (unsigned long long)diskgeometry.DiskSize.QuadPart / (unsigned long long)diskgeometry.Geometry.BytesPerSector;
}

ULONG DiskWriter_windows::deviceNumberFromName(const QString &device)
{
    QString volumename = "\\\\.\\" + device;
    HANDLE h = ::CreateFile(volumename.toStdWString().c_str(), 0, 0, NULL, OPEN_EXISTING, 0, NULL);

    STORAGE_DEVICE_NUMBER info;
    DWORD bytesReturned =  0;

    ::DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &info, sizeof(info), &bytesReturned, NULL);
    return info.DeviceNumber;
}
