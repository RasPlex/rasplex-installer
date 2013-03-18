#include "xmlhandler.h"

bool xmlHandler::startElement(const QString&, const QString &localName, const QString &, const QXmlAttributes &atts)
{
    if (localName == "link") {
        QString link;
        for (int i = 0; i < atts.count(); i++) {
            if (atts.localName(i) == "href") {
                link = atts.value(i);
                int n = link.lastIndexOf("download");
                if (n < 0) {
                    return true;
                }

                link = link.remove(n-1, link.size());
            }
        }
        if (link.contains("release") && link.endsWith("img.gz")) {
            releaseLinks += link;
        }
        if (link.contains("autoupdate") && link.endsWith("tar.bz2")) {
            upgradeLinks += link;
        }
    }
    return true;
}
