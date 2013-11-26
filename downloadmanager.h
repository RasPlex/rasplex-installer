#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QNetworkAccessManager>

class QUrl;

class DownloadManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = 0);

    QByteArray readAll();
    QNetworkReply* get(const QUrl& url, qlonglong size = -1);

signals:
    void downloadComplete(const QByteArray&);
    void partialData(const QByteArray&, qlonglong idx, qlonglong total);

private slots:
    void handleGetFinished(QNetworkReply*);
    void handleReadyRead();

private:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData);
    qlonglong bytesDownloaded;
    QNetworkReply* latestReply;

    enum {
        RESPONSE_OK = 200,
        RESPONSE_PARTIAL = 206,
        RESPONSE_FOUND = 302,
        RESPONSE_REDIRECT = 307,
        RESPONSE_BAD_REQUEST = 400,
        RESPONSE_NOT_FOUND = 404
    };

    void handlePartial(QNetworkReply *reply);
    void extractByteOffsetsFromContentLength(qlonglong &first, qlonglong &last, qlonglong &total, QString s);
    QByteArray rangeToByteArray(qlonglong first, qlonglong last)
    { return "bytes=" + QByteArray::number(first) + "-" + QByteArray::number(last); }

};

#endif // DOWNLOADMANAGER_H
