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
#include "confighandler_windows.h"
#elif defined(Q_OS_UNIX)
#include "diskwriter_unix.h"
#include "confighandler_unix.h"
#endif

// TODO: Get chunk size from server, or whatever
#define CHUNKSIZE 1*1024*1024

Installer::Installer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Installer),
    manager(new DownloadManager(this)),
    state(STATE_IDLE),
    bytesDownloaded(0),
    imageHash(QCryptographicHash::Md5)
{
    ui->setupUi(this);

#if defined(Q_OS_WIN)
    diskWriter = new DiskWriter_windows(this);
    configHandler = new ConfigHandler_windows();
#elif defined(Q_OS_UNIX)
    diskWriter = new DiskWriter_unix(this);
    configHandler = new ConfigHandler_unix();
#endif

    refreshDeviceList();

    connect(manager, SIGNAL(downloadComplete(QByteArray)), this, SLOT(handleFinishedDownload(QByteArray)));
    connect(manager, SIGNAL(partialData(QByteArray,qlonglong)),
            this, SLOT(handlePartialData(QByteArray,qlonglong)));
    connect(ui->linksButton, SIGNAL(clicked()), this, SLOT(updateLinks()));
    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(downloadImage()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(ui->cancelButton, SIGNAL(clicked()), diskWriter, SLOT(cancelWrite()));
    connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(getImageFileNameFromUser()));
    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(writeImageToDevice()));
    connect(diskWriter, SIGNAL(bytesWritten(int)), ui->progressBar, SLOT(setValue(int)));
    connect(ui->refreshDeiceListButton,SIGNAL(clicked()),this,SLOT(refreshDeviceList()));
    connect(ui->hdmiOutputButton, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvOutputButton, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_0, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_1, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_2, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_3, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));

    ui->videoGroupBox->setVisible(configHandler->implemented());
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
    delete diskWriter;
    delete configHandler;
}

void Installer::refreshDeviceList()
{
    qDebug() << "Refreshing device list";
    ui->removableDevicesComboBox->clear();

    QStringList devNames = diskWriter->getRemovableDeviceNames();
    QStringList friendlyNames = diskWriter->getUserFriendlyNames(devNames);

    for (int i = 0; i < devNames.size(); i++) {
        ui->removableDevicesComboBox->addItem(friendlyNames[i], devNames[i]);
    }

    ui->removableDevicesComboBox->setCurrentIndex(ui->removableDevicesComboBox->count()-1);
}

void Installer::cancel()
{
    if (!isCancelled)
        isCancelled = true;

    reset();
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
    foreach (xmlHandler::DownloadInfo info, handler->releases) {
        QUrl link = info.url;
        QString version = link.toString();

        // Remove full url and ".img.gz"
        int idx = version.lastIndexOf('-');
        version.remove(0, idx+1);
        version.chop(16);

        ui->releaseLinks->insertItem (0, version, link);
    }

    // Add current, bleeding, and experimental
    ui->releaseLinks->insertItem(0, "experimental", handler->experimental.url);
    ui->releaseLinks->insertItem(0, "bleeding", handler->bleeding.url);
    ui->releaseLinks->insertItem(0, "current", handler->current.url);
    ui->releaseLinks->setCurrentIndex(0);

    // Add upgrade links
    foreach (xmlHandler::DownloadInfo info, handler->autoupdates) {
        QUrl link = info.url;
        QString version = link.toString();
        // Remove full url and ".tar.bz2"
        int idx = version.lastIndexOf('-');
        version.remove(0, idx+1);
        version.chop(17);
        ui->upgradeLinks->addItem(version, link);
    }

    reset();

    /* Save the data */
    QFile file("sf_data.xml");
    if (file.open(QIODevice::WriteOnly | QFile::Truncate)) {
        file.write(data);
        file.close();
    }
}

void Installer::reset()
{
    state = STATE_IDLE;
    bytesDownloaded = 0;
    if (imageFile.isOpen()) {
        imageFile.close();
    }
    ui->releaseLinks->setEnabled(true);
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
    ui->releaseLinks->setEnabled(false);
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

bool Installer::isChecksumValid()
{
    QByteArray referenceSum, downloadSum;
    QCryptographicHash c(QCryptographicHash::Md5);

    // Calculate the md5 sum of the downloaded file
    // TODO: Calculate sum as we download (make it work)
    imageFile.open(QFile::ReadOnly);
    while (!imageFile.atEnd()) {
        c.addData(imageFile.read(4096));
    }
    downloadSum = c.result().toHex();
    imageFile.close();

    xmlHandler *handler = dynamic_cast<xmlHandler*>(xmlReader.contentHandler());
    if (handler == NULL) {
        return false;
    }

    // Find our reference sum
    QUrl selectedUrl = ui->releaseLinks->itemData(ui->releaseLinks->currentIndex()).toUrl();
    foreach (xmlHandler::DownloadInfo info, handler->releases) {
        if (info.url == selectedUrl) {
            referenceSum = info.md5sum;
            break;
        }
    }

    if (referenceSum.isEmpty() || downloadSum != referenceSum) {
        return false;
    }

    return true;
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

    /* Remove full path for display */
    int idx = filename.lastIndexOf('/');
    if (idx > 0) {
        filename.remove(0, idx+1);
    }
    ui->fileNameLabel->setText(filename);
}

void Installer::handleFinishedDownload(const QByteArray &data)
{
    switch (state) {
    case STATE_GETTING_LINKS:
        parseAndSetLinks(data);
        break;

    case STATE_GETTING_URL:{
        /* This should be it! */
        downloadUrl.append(data);
        downloadUrl = downloadUrl.trimmed();
        qDebug() << "Got url" << downloadUrl;
        int idx = ui->releaseLinks->findData(downloadUrl);
        ui->releaseLinks->setCurrentIndex(idx);
        reset();
        ui->downloadButton->click();
        break;
    }

    case STATE_DOWNLOADING_IMAGE:
        break;

    default:
        break;
    }
}

void Installer::handlePartialData(const QByteArray &data, qlonglong total)
{
    if (state == STATE_GETTING_URL) {
        downloadUrl.append(data);
        return;
    }

    if (state != STATE_DOWNLOADING_IMAGE) {
        /* What to do? */
        qDebug() << "Got unexpected data!";
        return;
    }

    imageFile.write(data);
    imageHash.addData(data);
    bytesDownloaded += data.size();

    // Update progress bar
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(bytesDownloaded);
    qDebug() << bytesDownloaded << "/" << total << "=" << (qreal)bytesDownloaded/total * 100 << "%";

    if (bytesDownloaded == total) {
        imageFile.close();
        qDebug() << "Download complete, verifying checksum";
        // Done! Let's check checksum!
        reset();
        if (!isChecksumValid()) {
            ui->messageBar->setText("Wrong md5 sum! Please re-download.");
        }
    }
}

void Installer::updateLinks()
{
    state = STATE_GETTING_LINKS;
    disableControls();

    QUrl url("http://sourceforge.net/api/file/index/project-id/1284489/atom");
    manager->get(url);
}

void Installer::downloadImage()
{
    state = STATE_DOWNLOADING_IMAGE;
    disableControls();
    QString selectedVersion = ui->releaseLinks->itemText(ui->releaseLinks->currentIndex());
    QUrl url = ui->releaseLinks->itemData(ui->releaseLinks->currentIndex()).toUrl();

    qDebug() << "Downloading" << url;
    // Handle special releases
    if (    selectedVersion == "bleeding" ||
            selectedVersion == "current" ||
            selectedVersion == "experimental") {
        ui->messageBar->setText("Getting URL");
        downloadUrl.clear();
        state = STATE_GETTING_URL;
    }
    else {
        // Try to find file name in url
        QString newFileName = url.toString().section('/',-2,-2);

        // Ask for final name
        newFileName = QFileDialog::getSaveFileName(this, tr("Save file"),
                                                   QDir::homePath()+"/"+newFileName);
        if (newFileName.isEmpty()) {
            reset();
            return;
        }

        if (imageFile.isOpen())
            imageFile.close();

        setImageFileName(newFileName);

        if (!imageFile.open(QFile::WriteOnly | QFile::Truncate)) {
            reset();
            return;
        }

        imageHash.reset();
    }
    manager->get(url);
}

void Installer::getImageFileNameFromUser()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open rasplex image", QDir::homePath());
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

    int idx = ui->removableDevicesComboBox->currentIndex();
    QString destination = ui->removableDevicesComboBox->itemData(idx).toString();

    QMessageBox::StandardButton ok = QMessageBox::warning(this, tr("Confirm write"),
                                                          "Are you sure you want to write to?\n"
                                                          + destination +
                                                          "\n\nThis might destroy your data!",
                                                          QMessageBox::Yes | QMessageBox::No,
                                                          QMessageBox::No);

    if (ok != QMessageBox::Yes) {
        reset();
        return;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(getUncompressedImageSize());
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

    // Make sure the config is updated
    selectVideoOutput();

    reset();
    qDebug() << "Writing done!";
}

void Installer::selectVideoOutput()
{
    if (!configHandler->implemented()) {
        return;
    }

    int idx = ui->removableDevicesComboBox->currentIndex();
    if (!configHandler->mount(ui->removableDevicesComboBox->itemData(idx).toString())) {
        return;
    }

    if (ui->hdmiOutputButton->isChecked()) {
        configHandler->changeSetting("hdmi_force_hotplug", 1);
    }
    else {
        configHandler->changeSetting("hdmi_force_hotplug", 0);
        if (ui->sdtvMode_0->isChecked()) {
            configHandler->changeSetting("sdtv_mode", 0);
        } else if (ui->sdtvMode_1->isChecked()) {
            configHandler->changeSetting("sdtv_mode", 1);
        } else if (ui->sdtvMode_2->isChecked()) {
            configHandler->changeSetting("sdtv_mode", 2);
        } else if (ui->sdtvMode_3->isChecked()) {
            configHandler->changeSetting("sdtv_mode", 3);
        } else {
            // Shouldn't be possible!
        }
    }
    ui->messageBar->setText("Settings changed");
    configHandler->unMount();
}
