#include "deviceenumerator_windows.h"

#include <windows.h>

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
    // Not implemented yet
    return devices;
}
