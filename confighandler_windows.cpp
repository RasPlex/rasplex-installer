#include "confighandler_windows.h"

ConfigHandler_windows::~ConfigHandler_windows()
{
    unMount();
}

bool ConfigHandler_windows::implemented()
{
    return false;
}

bool ConfigHandler_windows::mount(const QString &device)
{
    return false;
}

void ConfigHandler_windows::unMount()
{
}
