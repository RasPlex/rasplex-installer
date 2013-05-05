#include "confighandler_unix.h"

ConfigHandler_unix::~ConfigHandler_unix()
{
    unMount();
}

bool ConfigHandler_unix::implemented()
{
    return false;
}

bool ConfigHandler_unix::mount(const QString &device)
{
    return false;
}

void ConfigHandler_unix::unMount()
{
}

