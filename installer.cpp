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
#include <QMessageBox>

#if defined(Q_OS_WIN)
#include "diskwriter_windows.h"
#elif defined(Q_OS_UNIX)
#include "diskwriter_unix.h"
#endif

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

#if defined(Q_OS_WIN)
    diskWriter = new DiskWriter_windows(this);
#elif defined(Q_OS_UNIX)
    diskWriter = new DiskWriter_unix(this);
#endif

    refreshDeviceList();

    connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileListReply(QNetworkReply*)));
    connect(ui->linksButton, SIGNAL(clicked()), this, SLOT(updateLinks()));
    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(getDownloadLink()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(getImageFileNameFromUser()));
    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(writeImageToDevice()));
    connect(diskWriter, SIGNAL(bytesWritten(int)), ui->progressBar, SLOT(setValue(int)));
    connect(ui->refreshDeiceListButton,SIGNAL(clicked()),this,SLOT(refreshDeviceList()));
    ui->videoGroupBox->setVisible(false);
    ui->upgradeLabel->setVisible(false);
    ui->upgradeLinks->setVisible(false);

    ui->hdmiOutputButton->setChecked(true);
    isCancelled = false;
    xmlHandler *handler = new xmlHandler;
    xmlReader.setContentHandler(handler);

    setImageFileName("");
    ui->writeButton->setEnabled(false);

    QFile file("sf_data.xml");
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

void Installer::refreshDeviceList()
{
    qDebug() << "Refreshing device list";
    ui->removableDevicesComboBox->clear();
    ui->removableDevicesComboBox->addItems(diskWriter->getRemovableDeviceNames());
    // ui->removableDevicesComboBox->addItems(diskWriter->getUserFriendlyNamesRemovableDevices(devices));
    ui->removableDevicesComboBox->setCurrentIndex(ui->removableDevicesComboBox->count()-1);
}

void Installer::cancel()
{

    if (state == STATE_WRITING_IMAGE)
        diskWriter->cancelWrite();

    if (!isCancelled)
        isCancelled = true;

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
    // Add release links

    foreach (QString link, handler->releaseLinks) {
        QString version = link;
        // Remove full url and ".img.gz"
        int idx = link.lastIndexOf('-');
        version.remove(0, idx+1);
        version.chop(7);


        ui->releaseLinks->insertItem (0,version, link);
    }

    // Add current and bleeding
    ui->releaseLinks->insertItem(0, "bleeding", handler->bleeding);
    ui->releaseLinks->insertItem(0, "current", handler->current);
    ui->releaseLinks->setCurrentIndex(0);

    // Add upgrade links
    foreach (QString link, handler->upgradeLinks) {
        QString version = link;
        // Remove full url and ".tar.bz2"
        int idx = link.lastIndexOf('-');
        version.remove(0, idx+1);
        version.chop(8);
        ui->upgradeLinks->addItem(version, link);
    }

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
    ui->writeButton->setEnabled( !ui->fileNameLabel->text().isEmpty());
    ui->loadButton->setEnabled(true);
    ui->hdmiOutputButton->setEnabled(true);
    ui->sdtvOutputButton->setEnabled(true);
    ui->cancelButton->setEnabled(true);
    ui->refreshDeiceListButton->setEnabled(true);
    ui->removableDevicesComboBox->setEnabled(true);
    isCancelled = false;
    ui->messageBar->setText("Please download and select an image to write.");
}

void Installer::disableControls()
{
    ui->linksButton->setEnabled(false);
    ui->downloadButton->setEnabled(false);
    ui->writeButton->setEnabled(false);
    ui->loadButton->setEnabled(false);
    ui->hdmiOutputButton->setEnabled(false);
    ui->sdtvOutputButton->setEnabled(false);
    ui->cancelButton->setEnabled(true);
    ui->refreshDeiceListButton->setEnabled(false);
    ui->removableDevicesComboBox->setEnabled(false);

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

// From http://www.gamedev.net/topic/591402-gzip-uncompressed-file-size/
// Might not be portable!
unsigned int Installer::getUncompressedImageSize()
{
    FILE					*file;
    unsigned int			len;
    unsigned char			bufSize[4];
    unsigned int			fileSize;

    file = fopen(imageFileName.toStdString().c_str(), "rb");
    if (file == NULL)
        return 0;

    if (fseek(file, -4, SEEK_END) != 0)
        return 0;

    len = fread(&bufSize[0], sizeof(unsigned char), 4, file);
    if (len != 4) {
        fclose(file);
        return 0;
    }

    fileSize = (unsigned int) ((bufSize[3] << 24) | (bufSize[2] << 16) | (bufSize[1] << 8) | bufSize[0]);
    qDebug() << "Uncompressed FileSize:" << fileSize;

    fclose(file);
    return fileSize;
}

void Installer::setImageFileName(QString filename)
{
    if (imageFile.isOpen()) {
        qDebug() << "Tried to change filename while imageFile was open!";
        return;
    }
    imageFileName = filename;
    imageFile.setFileName(imageFileName);

    int idx = filename.lastIndexOf('/');
    if (idx > 0) {
        filename.remove(0, idx+1);
    }
    ui->fileNameLabel->setText(filename);
}

void Installer::updateLinks()
{
    state = STATE_GETTING_LINKS;
    disableControls();

    QUrl url("http://sourceforge.net/api/file/index/project-id/1284489/atom");
    manager.get(QNetworkRequest(url));
}

void Installer::getDownloadLink()
{
    state = STATE_GETTING_URL;
    disableControls();

    QString link = ui->releaseLinks->itemData(ui->releaseLinks->currentIndex()).toString();
    if (!link.contains("bleeding") && !link.contains("current")) {
        // Try to find file name in url
        int idx = link.lastIndexOf('/');
        if (idx > 0) {
            QString newFileName = link;
            setImageFileName(newFileName.remove(0, idx+1));
        }
    }

    QUrl url(link + "/download");
    manager.get(createRequest(url, 0, CHUNKSIZE));
}

void Installer::downloadImage(QNetworkReply *reply)
{
    // TODO: Sanity checks!
    state = STATE_DOWNLOADING_IMAGE;
    qlonglong first, last, total;
    qlonglong contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    QByteArray contentRange = reply->rawHeader("Content-Range");

    // Do some magic. \todo Make it not report error on uncompressed iages
    extractByteOffsetsFromContentLength(first, last, total, QString(contentRange));

    // This is the first valid packet, save it!
    qDebug() << "Final url:" << downloadUrl << total;

    // Bleeding and current are special
    if (downloadUrl.toString().contains("bleeding") || downloadUrl.toString().contains("current")) {
        QString link = reply->readAll();
        link.chop(strlen("/download\n"));
        int idx = ui->releaseLinks->findData(link);
        ui->releaseLinks->setCurrentIndex(idx);
        reset();
        ui->downloadButton->click();
        return;
    }

    if (!imageFile.isOpen() && !imageFile.open(QFile::ReadWrite)) {
        reset();
        return;
    }

    imageFile.write(reply->readAll());
    bytesDownloaded = contentLength;

    while (last <= total && ! isCancelled) {
        manager.get(createRequest(downloadUrl, last+1, (last+CHUNKSIZE >= total ? total-1 : last+CHUNKSIZE)));
        last += CHUNKSIZE;
    }
}

void Installer::fileListReply(QNetworkReply *reply)
{
    QByteArray replyData;
    if (isCancelled) {
        isCancelled = false;
        ui->progressBar->setValue(ui->progressBar->minimum());
        reset();
    }

    if(reply->error() == QNetworkReply::NoError)
    {
        QUrl redirectionUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();

        switch (state) {
        case STATE_GETTING_LINKS:
            if (responseCode == RESPONSE_OK && reply->isReadable()) {
                replyData = reply->readAll();
                parseAndSetLinks(replyData);
                QFile file("sf_data.xml");
                if (file.open(QFile::WriteOnly)) {
                    file.write(replyData);
                    file.close();
                }
            }
            reset();
            break;
        case STATE_GETTING_URL:
            ui->messageBar->setText("Determining mirror URL...");
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
            ui->messageBar->setText("Downloading image...");
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

void Installer::getImageFileNameFromUser()
{
    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.isNull()) {
        return;
    }
    setImageFileName(filename);
    ui->writeButton->setEnabled(true);
    qDebug() << imageFileName;
}

void Installer::writeImageToDevice()
{
    disableControls();

    QMessageBox::StandardButton ok = QMessageBox::warning(this, tr("Confirm write"),
                                                          "Are you sure?\n"
                                                          "This might destroy your data!",
                                                          QMessageBox::Yes | QMessageBox::No,
                                                          QMessageBox::No);

    if (ok != QMessageBox::Yes) {
        reset();
        return;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(getUncompressedImageSize());

    // TODO: make portable
    QString destination = ui->removableDevicesComboBox->currentText();

    ui->messageBar->setText("Writing image to "+destination);

    if (destination.isNull()) {
        reset();
        return;
    }

    // DiskWriter will re-open the image file.
    if (imageFile.isOpen()) {
        imageFile.close();
    }

    if (diskWriter->open(destination) < 0) {
        qDebug() << "Failed to open output device";
        reset();
        ui->messageBar->setText("Unable to open "+destination+". Are you root?");
        return;
    }

    if (!diskWriter->writeCompressedImageToRemovableDevice(imageFileName)) {
        qDebug() << "Writing failed";
        reset();
        return;
    }

    reset();
    qDebug() << "Writing done!";
}
