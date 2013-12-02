#include "diskwriter_windows.h"

#include <QDebug>
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

    hVolume = getHandleOnVolume(devString, GENERIC_WRITE);
    if (hVolume == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (!getLockOnVolume(hVolume)) {
        close();
        return false;
    }

    if (isVolumeMounted(hVolume) && !unmountVolume(hVolume)) {
        close();
        return false;
    }

    hRawDisk = getHandleOnDevice(devString, GENERIC_WRITE);
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

bool DiskWriter_windows::write(const char *data, qint64 size)
{
    DWORD byteswritten;
    bool ok;
    ok = WriteFile(hRawDisk, data, size, &byteswritten, NULL);
    if (!ok) {
        wchar_t *errormessage=NULL;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(), 0, (LPWSTR)&errormessage, 0, NULL);
        QString errText = QString::fromUtf16((const ushort *)errormessage);
        QMessageBox::critical(NULL, QObject::tr("Write Error"),
                              QObject::tr("An error occurred when attempting to write data to handle.\n"
                                          "Error %1: %2").arg(GetLastError()).arg(errText));
        LocalFree(errormessage);
        QMessageBox::information(this, tr(" "), "Image failed to write : (");
    }else
       QMessageBox::information(this, tr(" "), "Image written successfully!");

    return ok;
}

// Adapted from win32 DiskImager
HANDLE DiskWriter_windows::getHandleOnDevice(const QString& device, DWORD access) const
{
    HANDLE hDevice;
    QString devicename = QString("\\\\.\\PhysicalDrive%1").arg(deviceNumberFromName(device));
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
HANDLE DiskWriter_windows::getHandleOnVolume(const QString &volume, DWORD access) const
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
bool DiskWriter_windows::getLockOnVolume(HANDLE handle) const
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
bool DiskWriter_windows::removeLockOnVolume(HANDLE handle) const
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
bool DiskWriter_windows::unmountVolume(HANDLE handle) const
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
bool DiskWriter_windows::isVolumeMounted(HANDLE handle) const
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(handle, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &junk, NULL);
    return (bResult);
}

ULONG DiskWriter_windows::deviceNumberFromName(const QString &device) const
{
    QString volumename = "\\\\.\\" + device;
    HANDLE h = ::CreateFile(volumename.toStdWString().c_str(), 0, 0, NULL, OPEN_EXISTING, 0, NULL);

    STORAGE_DEVICE_NUMBER info;
    DWORD bytesReturned =  0;

    ::DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &info, sizeof(info), &bytesReturned, NULL);
    return info.DeviceNumber;
}
