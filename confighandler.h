#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <QString>

class ConfigHandler
{
public:
    ConfigHandler() {}
    virtual ~ConfigHandler() {}

    // OS dependant stuff
    virtual bool implemented() = 0;
    virtual bool mount(const QString &device) = 0;
    virtual void unMount() = 0;

    // Portable stuff
    void setConfigFilename(const QString &filename);
    QString configFilename() const;
    bool changeSetting(const char * setting, const int value) const;

protected:
    QString filename;
};

#endif // SETTINGSHANDLER_H
