#ifndef DEVICEENUMERATOR_UNIX_H
#define DEVICEENUMERATOR_UNIX_H

#include "deviceenumerator.h"
#include <QStringList>

class DeviceEnumerator_unix : public DeviceEnumerator
{
public:
    QStringList getRemovableDeviceNames() const;
    QStringList getUserFriendlyNames(const QStringList& devices) const;

private:
    bool checkIsMounted(const QString& device) const;
    bool checkIfUSB(const QString& device) const;
    QStringList getDeviceNamesFromSysfs() const;
#if defined(Q_OS_LINUX)
    qint64 driveSize(const QString &device) const;
    QStringList getPartitionsInfo(const QString &device) const;
#endif
};

#endif // DEVICEENUMERATOR_UNIX_H
