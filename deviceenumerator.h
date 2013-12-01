#ifndef DEVICEENUMERATOR_H
#define DEVICEENUMERATOR_H

#include <QStringList>

/* Interface class */
class DeviceEnumerator
{
public:
    virtual ~DeviceEnumerator() {}
    virtual QStringList getRemovableDeviceNames() const = 0;
    virtual QStringList getUserFriendlyNames(const QStringList& devices) const = 0;

};

#endif // DEVICEENUMERATOR_H
