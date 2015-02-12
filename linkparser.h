#ifndef LINKPARSER_H
#define LINKPARSER_H

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

class QByteArray;
class LinkParserPrivate;

// POD struct
struct ReleaseData {
    ReleaseData(QMap<QString, QVariant> data) : values(data) {}
    QString operator[] (const QString &key) const
    { return values[key].toString(); }

    QMap<QString, QVariant> values;
};

struct DeviceData {
    DeviceData(QMap<QString, QVariant> data) : values(data) {}
    QString operator[] (const QString &key) const
    { return values[key].toString(); }

    QMap<QString, QVariant> values;
};

class LinkParser
{
public:
    LinkParser(const QByteArray &data);
    QList<ReleaseData> releases() const;

private:
    QList<ReleaseData> releaseData;
};

class DeviceParser
{
public:
    DeviceParser(const QByteArray &data);
    QList<DeviceData> devices() const;

private:
    QList<DeviceData> deviceData;
};


#endif // LINKPARSER_H
