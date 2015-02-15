#include "simplejsonparser.h"

#include <QByteArray>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QStandardPaths>
#else
#include <qjson/parser.h>
#endif

SimpleJsonParser::SimpleJsonParser(const QByteArray &data)
{
    QVariantList list;
    bool ok;

    #if QT_VERSION >= 0x050000
    list = QJsonDocument::fromJson(data).toVariant().toList();
    #else
    QJson::Parser parser;

    list = parser.parse(data, &ok).toList();
    Q_ASSERT(ok);
    #endif

    foreach (const QVariant &item, list) {
        ok = item.canConvert<QVariantMap>();
        Q_ASSERT(ok);
        dataStorage.append(item.toMap());
    }
}

JsonArray SimpleJsonParser::getJsonArray() const
{
    return dataStorage;
}
