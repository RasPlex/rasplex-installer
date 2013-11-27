#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = 0);
    ~DownloadManager();

    QNetworkReply* get(const QUrl& url);

signals:
    void downloadComplete(const QByteArray&);
    void partialData(const QByteArray, qlonglong total);

private slots:
    void handleGetFinished(QNetworkReply*);
    void handleReadyRead();

private:
    QNetworkAccessManager* manager;
    QNetworkReply* latestReply;

    enum {
        RESPONSE_OK = 200,
        RESPONSE_PARTIAL = 206,
        RESPONSE_FOUND = 302,
        RESPONSE_REDIRECT = 307,
        RESPONSE_BAD_REQUEST = 400,
        RESPONSE_NOT_FOUND = 404
    };
};

#endif // DOWNLOADMANAGER_H
