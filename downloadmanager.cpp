#include "downloadmanager.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <QDebug>

// TODO: Get chunk size from server, or whatever
#define CHUNKSIZE 1*1024*1024

DownloadManager::DownloadManager(QObject *parent) :
    QNetworkAccessManager(parent)
{
}

QNetworkReply* DownloadManager::get(const QUrl &url, qlonglong size)
{
    QNetworkRequest req(url);

    if (size >= CHUNKSIZE) {
        QByteArray range = rangeToByteArray(0, CHUNKSIZE);
        req.setRawHeader("Range", range);
        req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    }
    req.setRawHeader("User-Agent", "Wget/1.14 (linux-gnu)");
    req.setRawHeader("Connection", "keep-alive");

    qDebug() << "Getting" << url;
    foreach (QByteArray d, req.rawHeaderList()) {
        qDebug() << d << req.rawHeader(d);
    }

    return QNetworkAccessManager::get(req);
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
                qDebug() << "Redirected to" << redirectionUrl;

                foreach (QByteArray d, reply->rawHeaderList()) {
                    qDebug() << d << reply->rawHeader(d);
                }
                qDebug() << reply->readAll();
#if 0
                QNetworkRequest req = reply->request();
                req.setUrl(redirectionUrl);
                req.setAttribute();
                QNetworkAccessManager::get(req);
#else
                bytesDownloaded = 0;
                latestReply = get(redirectionUrl);
                connect(latestReply, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
#endif
            }
            else {
                qDebug() << "Redirected but no redirection url?!";
            }
            break;
        case RESPONSE_OK:
            qDebug() << "Downloaded" << reply->size() << "bytes from" << reply->url().toString();
            emit downloadComplete(reply->readAll());
            break;
        case RESPONSE_PARTIAL:
            handlePartial(reply);
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
    if(latestReply->error() == QNetworkReply::NoError)
    {
        int responseCode = latestReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();
        if (responseCode == 200) {
            QByteArray data = latestReply->readAll();
            qlonglong contentLength = latestReply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
            emit partialData(data, bytesDownloaded, contentLength);
            bytesDownloaded += data.size();
        }
    }
    else {
        qDebug() << "HandleReadyRead(): Something went wrong with the get request:" << latestReply->errorString();
        latestReply->deleteLater();
    }
}

QNetworkReply *DownloadManager::createRequest(QNetworkAccessManager::Operation op,
                                              const QNetworkRequest &request,
                                              QIODevice *outgoingData)
{
    QNetworkReply* reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
    if (op == QNetworkAccessManager::GetOperation) {
        connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleGetFinished(QNetworkReply*)));
    }

    return reply;
}

void DownloadManager::handlePartial(QNetworkReply* reply)
{
    qlonglong first, last, total;
    //qlonglong contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    QByteArray contentRange = reply->rawHeader("Content-Range");

    // Sanity check
    if (contentRange.isEmpty()) {
        qDebug() << "No Content-Range";
        return;
    }

    // Do some magic
    extractByteOffsetsFromContentLength(first, last, total, QString(contentRange));

    qDebug() << "Got" << last-first << "bytes of data, out of total" << total << "bytes";
    emit partialData(reply->readAll(), first, total);
}

void DownloadManager::extractByteOffsetsFromContentLength(qlonglong &first, qlonglong &last, qlonglong &total, QString s)
{
    // Remove unit (assume bytes)
    int idx = s.indexOf(' ');
    s.remove(0, idx+1);

    // Find first
    idx = s.indexOf('-');
    first = s.left(idx).toLongLong();
    s.remove(0, idx+1);

    // Find last
    idx = s.indexOf('/');
    last = s.left(idx).toLongLong();
    s.remove(0, idx+1);

    // Total
    total = s.toLongLong();
}
