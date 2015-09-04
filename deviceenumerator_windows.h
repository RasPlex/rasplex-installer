#ifndef DEVICEENUMERATOR_WINDOWS_H
#define DEVICEENUMERATOR_WINDOWS_H

#include "deviceenumerator.h"

#include <QStringList>

class DeviceEnumerator_windows : public DeviceEnumerator
{
public:
    QStringList getRemovableDeviceNames() const;
    QStringList getUserFriendlyNames(const QStringList& devices) const;

private:
    static quint64 getSizeOfDevice(const QString &device);
};

#endif // DEVICEENUMERATOR_WINDOWS_H
