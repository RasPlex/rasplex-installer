#ifndef CONFIGHANDLER_UNIX_H
#define CONFIGHANDLER_UNIX_H

#include "confighandler.h"

class ConfigHandler_unix : public ConfigHandler
{
public:
    ConfigHandler_unix() {}
    ~ConfigHandler_unix();

    bool implemented();
    bool mount(const QString &device);
    void unMount();
};

#endif // CONFIGHANDLER_UNIX_H
