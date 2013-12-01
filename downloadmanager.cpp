#include "downloadmanager.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <QDebug>

DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent),
    manager(new QNetworkAccessManager(this)),
    latestReply(NULL)
{
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleGetFinished(QNetworkReply*)));
}

DownloadManager::~DownloadManager()
{
    delete manager;
}

QNetworkReply* DownloadManager::get(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Wget/1.14 (linux-gnu)");
    req.setRawHeader("Connection", "keep-alive");
    qDebug() << "Getting" << url;

    latestReply = manager->get(req);
    return latestReply;
}

void DownloadManager::cancelDownload()
{
    if (latestReply)
        latestReply->abort();
}

void DownloadManager::handleGetFinished(QNetworkReply *reply)
{
    if(reply->error() == QNetworkReply::NoError)
    {
        QUrl redirectionUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();

        qDebug() << "Handling get request with response code" << responseCode;

        switch (responseCode) {
        case RESPONSE_FOUND:
            redirectionUrl = reply->header(QNetworkRequest::LocationHeader).toUrl();
        case RESPONSE_REDIRECT:
            if (redirectionUrl.isValid()) {
                qDebug() << reply->url() << "redirected to" << redirectionUrl;

                /* Make sure we don't send out unnecessary get requests */
                if (latestReply && latestReply->url() == redirectionUrl)
                    break;

                /* Prepare for download */
                get(redirectionUrl);
                connect(latestReply, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
            }
            else {
                qDebug() << "Redirected but no redirection url?!";
            }
            break;

        case RESPONSE_OK:
            qDebug() << "Downloaded" << reply->header(QNetworkRequest::ContentLengthHeader).toLongLong() << "bytes from" << reply->url().toString();
            emit downloadComplete(reply->readAll());
            break;

        case RESPONSE_PARTIAL:
            handleReadyRead();
            break;

        default:
            qDebug() << "Unhandled reply:" << responseCode;
            break;
        }
    }
    else {
        qDebug() << "Something went wrong with the get request:" << reply->errorString();
    }

    reply->deleteLater();
}

void DownloadManager::handleReadyRead()
{
    if (latestReply->error() == QNetworkReply::NoError)
    {
        int responseCode = latestReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();
        if (responseCode == 200) {           
            if (!latestReply->bytesAvailable()) {
                qDebug() << "ReadyRead() with 0 bytes available :S";
                return;
            }
            qlonglong total = latestReply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
            emit partialData(latestReply->readAll(), total);
        }
    }
    else {
        qDebug() << "HandleReadyRead(): Something went wrong with the get request:" << latestReply->errorString();
        latestReply->deleteLater();
    }
}
