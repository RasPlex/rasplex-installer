#include "xmlhandler.h"
#include <QDebug>

bool xmlHandler::startElement(const QString &, const QString &, const QString &qName, const QXmlAttributes &atts)
{
    if (qName == "media:content") {
        if (atts.value("type") == "application/x-gzip; charset=binary") {
            QString url = atts.value("url");

            // Filter out rasplexdev builds
            if (!url.contains("release") && !url.contains("experimental")) {
                return true;
            }

            // Work around bug in sourceforge data
            url.replace("/project/", "/projects/");

            info.url = url;
            info.filesize = atts.value("filesize").toUInt();
        }
        if (atts.value("type") == "text/plain; charset=us-ascii") {
            QString url = atts.value("url");

            // Work around bug in sourceforge data
            url.replace("/project/", "/projects/");

            if (url.contains("experimental")) {
                experimental.url = url;
                experimental.filesize = atts.value("filesize").toUInt();
            }
            if (url.contains("bleeding")) {
                bleeding.url = url;
                bleeding.filesize = atts.value("filesize").toUInt();
            }
            if (url.contains("current")) {
                current.url = url;
                current.filesize = atts.value("filesize").toUInt();
            }
        }
    }

    currentText.clear();
    return true;
}

bool xmlHandler::endElement(const QString &, const QString &, const QString &qName)
{
    if (qName == "media:content" &&
            info.url.isValid() &&
            info.filesize != 0 &&
            !info.md5sum.isEmpty()) {
        releases.append(info);
        clearInfo();
    }

    if (qName == "media:hash") {
        info.md5sum = qPrintable(currentText);
    }

    return true;
}

bool xmlHandler::characters(const QString &str)
{
    currentText += str;
    return true;
}

void xmlHandler::clearInfo()
{
    info.filesize = 0;
    info.md5sum = QByteArray();
    info.url = QUrl();
}
