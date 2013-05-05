#ifndef CONFIGHANDLER_WINDOWS_H
#define CONFIGHANDLER_WINDOWS_H

#include "confighandler.h"

class ConfigHandler_windows : public ConfigHandler
{
public:
    ConfigHandler_windows() {}
    ~ConfigHandler_windows();

    bool implemented();
    bool mount(const QString &device);
    void unMount();
};

#endif // CONFIGHANDLER_WINDOWS_H
