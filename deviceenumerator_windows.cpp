#include "deviceenumerator_windows.h"

#include <windows.h>

#include "diskwriter_windows.h"

// Adapted from http://skilinium.com/blog/?p=134
QStringList DeviceEnumerator_windows::getRemovableDeviceNames() const
{
    QStringList names;
    WCHAR *szDriveLetters;
    WCHAR szDriveInformation[1024];

    GetLogicalDriveStrings(1024, szDriveInformation);

    szDriveLetters = szDriveInformation;
    while (*szDriveLetters != '\0') {
        if (GetDriveType(szDriveLetters) == DRIVE_REMOVABLE) {
            names << QString::fromWCharArray(szDriveLetters);
        }
        szDriveLetters = &szDriveLetters[wcslen(szDriveLetters) + 1];
    }
    return names;
}

QStringList DeviceEnumerator_windows::getUserFriendlyNames(const QStringList &devices) const
{
    const QStringList suffixes = QStringList() << "bytes" << "kB" << "MB" << "GB" << "TB";
    QStringList names;
    foreach (const QString &dev, devices) {
        quint64 size = getSizeOfDevice(dev);
        if (size != 0) {
            int idx = 0;
            qreal fSize = size;
            while ((fSize > 1024.0) && ((idx + 1) < suffixes.size())) {
                ++idx;
                fSize /= 1024.0;
            }
            names << dev + " (" + QString::number(fSize, 'f', 2) + " " + suffixes[idx] + ")";
        }
        else {
            names << dev;
        }
    }

    return names;
}

quint64 DeviceEnumerator_windows::getSizeOfDevice(const QString &device)
{
    HANDLE handle = DiskWriter_windows::getHandleOnDevice(device, GENERIC_READ);
    if (handle == INVALID_HANDLE_VALUE) {
        return 0;
    }

    DWORD junk;
    quint64 size = 0;
    GET_LENGTH_INFORMATION lenInfo;
    bool ok = DeviceIoControl(handle,
                              IOCTL_DISK_GET_LENGTH_INFO,
                              NULL, 0,
                              &lenInfo, sizeof(lenInfo),
                              &junk,
                              (LPOVERLAPPED) NULL);
    if (ok) {
        size = lenInfo.Length.QuadPart;
    }
    CloseHandle(handle);

    return size;
}
