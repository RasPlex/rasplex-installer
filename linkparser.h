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

class LinkParser
{
public:
    LinkParser(const QByteArray &data);
    QList<ReleaseData> releases() const;

private:
    QList<ReleaseData> releaseData;
};

#endif // LINKPARSER_H
