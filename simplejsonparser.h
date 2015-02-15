#ifndef SimpleJsonParser_H
#define SimpleJsonParser_H

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

class QByteArray;

// POD struct
struct JsonArrayItem {
    JsonArrayItem(QMap<QString, QVariant> data) : values(data) {}
    QString operator[] (const QString &key) const
    { return values[key].toString(); }

    QMap<QString, QVariant> values;
};

typedef QList<JsonArrayItem> JsonArray;

class SimpleJsonParser
{
public:
    SimpleJsonParser(const QByteArray &data);
    JsonArray getJsonArray() const;

private:
    JsonArray dataStorage;
};

#endif // SimpleJsonParser_H
