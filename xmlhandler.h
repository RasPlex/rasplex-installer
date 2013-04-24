#ifndef XMLHANDLER_H
#define XMLHANDLER_H

#include <QXmlDefaultHandler>
#include <QString>
#include <QStringList>

class xmlHandler : public QXmlDefaultHandler
{
public:
    xmlHandler() {}
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);

    QStringList releaseLinks;
    QStringList upgradeLinks;
    QString bleeding;
    QString current;
};

#endif // XMLHANDLER_H
