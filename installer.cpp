#include "installer.h"
#include "ui_installer.h"
#include "xmlhandler.h"

#include <QString>
#include <QFile>
#include <QFileDialog>
#include <iostream>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>

// TODO: Get chunk size from server, or whatever
#define CHUNKSIZE 1*1024*1024

Installer::Installer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Installer),
    state(STATE_IDLE),
    bytesDownloaded(0)
{
    ui->setupUi(this);
    manager.setParent(this);

    connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileListReply(QNetworkReply*)));
    connect(ui->linksButton, SIGNAL(clicked()), this, SLOT(updateLinks()));
    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(getDownloadLink()));

    xmlHandler *handler = new xmlHandler;
    xmlReader.setContentHandler(handler);

    imageFile.setFileName("foo.img.gz");

    QFile file("foo.xml");
    if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        parseAndSetLinks(data);
    }
    else {
        updateLinks();
    }
}

Installer::~Installer()
{
    delete ui;
}

void Installer::parseAndSetLinks(const QByteArray &data)
{
    QXmlInputSource source;
    source.setData(data);

    bool ok = xmlReader.parse(source);
    if (!ok) {
        std::cout << "Parsing failed." << std::endl;
    }

    xmlHandler *handler = dynamic_cast<xmlHandler*>(xmlReader.contentHandler());
    if (handler == NULL) {
        return;
    }

    ui->releaseLinks->clear();
    ui->upgradeLinks->clear();
    ui->releaseLinks->addItems(handler->releaseLinks);
    ui->upgradeLinks->addItems(handler->upgradeLinks);

    reset();
}

void Installer::saveAndUpdateProgress(QNetworkReply *reply)
{
    qlonglong first, last, total;
    qlonglong contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    QByteArray contentRange = reply->rawHeader("Content-Range");

    // Sanity check
    if (contentRange.isEmpty()) {
        reset();
        return;
    }

    // Do some magic
    extractByteOffsetsFromContentLength(first, last, total, QString(contentRange));

    // Save data
    imageFile.seek(first);
    imageFile.write(reply->readAll());
    bytesDownloaded += contentLength;

    // Update progress bar
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(bytesDownloaded);
    qDebug() << bytesDownloaded << "/" << total << "=" << (qreal)bytesDownloaded/total * 100 << "%";

    if (bytesDownloaded == total) {
        // Done!
        reset();
    }
}

void Installer::extractByteOffsetsFromContentLength(qlonglong &first, qlonglong &last, qlonglong &total, QString s)
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

void Installer::reset()
{
    state = STATE_IDLE;
    bytesDownloaded = 0;
    if (imageFile.isOpen()) {
        imageFile.close();
    }
    ui->linksButton->setEnabled(true);
    ui->downloadButton->setEnabled(true);
}

QByteArray Installer::rangeByteArray(qlonglong first, qlonglong last)
{
    return QByteArray("bytes=" + QByteArray::number(first) + "-" + QByteArray::number(last));
}

QNetworkRequest Installer::createRequest(QUrl &url, qlonglong first, qlonglong last)
{
    QNetworkRequest req(url);
    QByteArray range = rangeByteArray(first, last);
    qDebug() << range;
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    req.setRawHeader("Range", range);
    req.setRawHeader("User-Agent", "Wget/1.14 (linux-gnu)");
    return req;
}

void Installer::updateLinks()
{
    ui->linksButton->setEnabled(false);
    ui->downloadButton->setEnabled(false);
    state = STATE_GETTING_LINKS;
    QUrl url("http://sourceforge.net/api/file/index/project-id/1284489/atom");
    manager.get(QNetworkRequest(url));
}

void Installer::getDownloadLink()
{
    state = STATE_GETTING_URL;

    ui->linksButton->setEnabled(false);
    ui->downloadButton->setEnabled(false);

    QUrl url(ui->releaseLinks->currentText() + "/download");

    manager.get(createRequest(url, 0, CHUNKSIZE));
}

void Installer::downloadImage(QNetworkReply *reply)
{
    // TODO: Sanity checks!
    state = STATE_DOWNLOADING_IMAGE;
    qlonglong first, last, total;
    qlonglong contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    QByteArray contentRange = reply->rawHeader("Content-Range");

    // Do some magic
    extractByteOffsetsFromContentLength(first, last, total, QString(contentRange));

    // This is the first valid packet, save it!
    qDebug() << "Final url:" << downloadUrl << total;
    if (!imageFile.isOpen() && !imageFile.open(QFile::ReadWrite)) {
        reset();
        return;
    }
    imageFile.write(reply->readAll());
    bytesDownloaded = contentLength;

    while (last <= total) {
        manager.get(createRequest(downloadUrl, last+1, (last+CHUNKSIZE >= total ? total-1 : last+CHUNKSIZE)));
        last += CHUNKSIZE;
    }
}

void Installer::fileListReply(QNetworkReply *reply)
{
    QByteArray replyData;
    if(reply->error() == QNetworkReply::NoError)
    {
        QUrl redirectionUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();

        switch (state) {
        case STATE_GETTING_LINKS:
            if (responseCode == RESPONSE_OK && reply->isReadable()) {
                parseAndSetLinks(reply->readAll());
                QFile file("foo.xml"); // TODO: change file name
                if (file.open(QFile::WriteOnly)) {
                    file.write(replyData);
                    file.close();
                }
            }
            reset();
            break;
        case STATE_GETTING_URL:
            if (redirectionUrl.isValid()) {
                downloadUrl = redirectionUrl;
                QNetworkRequest request = reply->request();
                request.setUrl(redirectionUrl);
                manager.get(request);
                qDebug() << "Redirected to:" << redirectionUrl;
                break;
            }

            // Start pipelined download
            downloadImage(reply);
            break;
        case STATE_DOWNLOADING_IMAGE:
            switch (responseCode) {
            case RESPONSE_FOUND:
            case RESPONSE_REDIRECT:
                // Shouldn't happen!
                break;
            case RESPONSE_OK:
                // Probably a small file
                imageFile.write(reply->readAll());
                ui->progressBar->setValue(ui->progressBar->maximum());
                reset();
                break;
            case RESPONSE_PARTIAL:
                saveAndUpdateProgress(reply);
                break;
            default:
                qDebug() << "Unhandled reply:" << responseCode;
                break;
            }
            break; // downloading image
        default:
            break;
        }
    }

    reply->deleteLater();
}
