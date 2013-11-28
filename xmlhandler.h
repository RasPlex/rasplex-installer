#ifndef XMLHANDLER_H
#define XMLHANDLER_H

#include <QXmlDefaultHandler>
#include <QString>
#include <QStringList>
#include <QUrl>

class xmlHandler : public QXmlDefaultHandler
{
public:
    xmlHandler() {}
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);

    struct DownloadInfo {
        QUrl url;
        uint filesize;
        QByteArray md5sum;
    };

    QList<DownloadInfo> releases;
    QList<DownloadInfo> autoupdates;
    DownloadInfo experimental;
    DownloadInfo bleeding;
    DownloadInfo current;

private:
    DownloadInfo info;
    QString currentText;

    void clearInfo();
};



#endif // XMLHANDLER_H
