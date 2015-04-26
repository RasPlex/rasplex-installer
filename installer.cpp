#include "installer.h"
#include "ui_installer.h"

#include <QDebug>
#include <QString>
#include <QFile>
#include <QFileDialog>
#include <iostream>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QThread>
#include <QPlainTextEdit>

#if defined(Q_OS_WIN)
#include "diskwriter_windows.h"
#include "deviceenumerator_windows.h"
#include "confighandler_windows.h"
#elif defined(Q_OS_UNIX)
#include "diskwriter_unix.h"
#include "deviceenumerator_unix.h"
#include "confighandler_unix.h"
#endif
#include "simplejsonparser.h"

const QString Installer::m_serverUrl = "updater.rasplex.com";
//const QString Installer::m_serverUrl = "localhost:8080"; // for testing

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
    diskWriter = new DiskWriter_windows();
    devEnumerator = new DeviceEnumerator_windows();
    configHandler = new ConfigHandler_windows();
#elif defined(Q_OS_UNIX)
    diskWriter = new DiskWriter_unix();
    devEnumerator = new DeviceEnumerator_unix();
    configHandler = new ConfigHandler_unix();
#endif
    diskWriterThread = new QThread(this);
    diskWriter->moveToThread(diskWriterThread);
    connect(diskWriterThread, SIGNAL(finished()), diskWriter, SLOT(deleteLater()));
    connect(this, SIGNAL(proceedToWriteImageToDevice(QString,QString)),
            diskWriter, SLOT(writeCompressedImageToRemovableDevice(QString,QString)));
    connect(diskWriter, SIGNAL(bytesWritten(int)), ui->progressBar, SLOT(setValue(int)));
    connect(diskWriter, SIGNAL(syncing()), this, SLOT(writingSyncing()));
    connect(diskWriter, SIGNAL(finished()), this, SLOT(writingFinished()));
    connect(diskWriter, SIGNAL(error(QString)), this, SLOT(reset(QString)));
    diskWriterThread->start();

    refreshDeviceList();

    connect(manager, SIGNAL(downloadComplete(QByteArray)), this, SLOT(handleFinishedDownload(QByteArray)));
    connect(manager, SIGNAL(partialData(QByteArray,qlonglong)),
            this, SLOT(handlePartialData(QByteArray,qlonglong)));
    connect(ui->linksButton, SIGNAL(clicked()), this, SLOT(updateDevices()));
    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(downloadImage()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancel()));
    connect(ui->cancelButton, SIGNAL(clicked()), manager, SLOT(cancelDownload()));
    connect(ui->loadButton, SIGNAL(clicked()), this, SLOT(getImageFileNameFromUser()));
    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(writeImageToDevice()));

    connect(ui->refreshDeiceListButton,SIGNAL(clicked()),this,SLOT(refreshDeviceList()));

    connect(ui->hdmiOutputButton, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvOutputButton, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_0, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_1, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_2, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));
    connect(ui->sdtvMode_3, SIGNAL(clicked()), this, SLOT(selectVideoOutput()));

    connect(ui->releaseLinks, SIGNAL(currentIndexChanged(int)), ui->releaseNotes, SLOT(setCurrentIndex(int)));
    connect(ui->deviceSelectBox, SIGNAL(currentIndexChanged(int)), this, SLOT(getDeviceReleases(int)));




    ui->videoGroupBox->setVisible(configHandler->implemented());
    adjustSize();

    ui->hdmiOutputButton->setChecked(true);
    isCancelled = false;


    setImageFileName("");
    ui->writeButton->setEnabled(false);

    updateDevices();
}

Installer::~Installer()
{
    delete ui;
    diskWriter->cancelWrite();
    diskWriterThread->quit();
    diskWriterThread->wait();
    delete diskWriterThread;
    delete devEnumerator;
    delete configHandler;
}

void Installer::refreshDeviceList()
{
    qDebug() << "Refreshing device list";
    ui->removableDevicesComboBox->clear();

    QStringList devNames = devEnumerator->getRemovableDeviceNames();
    QStringList friendlyNames = devEnumerator->getUserFriendlyNames(devNames);

    for (int i = 0; i < devNames.size(); i++) {
        ui->removableDevicesComboBox->addItem(friendlyNames[i], devNames[i]);
    }

    ui->removableDevicesComboBox->setCurrentIndex(ui->removableDevicesComboBox->count()-1);
}

void Installer::cancel()
{
    if (!isCancelled)
        isCancelled = true;

    diskWriter->cancelWrite();
    ui->cancelButton->setEnabled(false);
    reset();
}

void Installer::parseAndSetSupportedDevices(const QByteArray &data)
{
    qDebug() << "Devices:" << data;
    SimpleJsonParser parser(data);

    ui->deviceSelectBox->clear();

    JsonArray devices = parser.getJsonArray();
    for (JsonArray::const_iterator it = devices.constBegin();
         it != devices.constEnd(); it++) {
        QString deviceName = (*it)["name"];
        QString deviceId = (*it)["id"];
        ui->deviceSelectBox->insertItem(0, deviceName ,deviceId);
    }

    reset();
    getDeviceReleases(ui->deviceSelectBox->currentIndex());
}

void Installer::parseAndSetLinks(const QByteArray &data)
{
    SimpleJsonParser parser(data);
    qDebug()<< "Links:" << data;

    ui->releaseLinks->clear();

    /* Clear all release notes */
    while (ui->releaseNotes->count()) {
        QWidget* widget = ui->releaseNotes->widget(0);
        ui->releaseNotes->removeWidget(widget);
        widget->deleteLater();
    }

    JsonArray releases = parser.getJsonArray();
    for (JsonArray::const_iterator it = releases.constBegin();
         it != releases.constEnd(); it++) {
        QString releaseName = (*it)["version"];
        QString url = (*it)["install_url"];
        QString notes = (*it)["notes"];
        QString localchecksum = (*it)["install_sum"];

        checksumMap[releaseName] = localchecksum;

        ui->releaseLinks->insertItem(0, releaseName, url);

        /* Add release note */
        QPlainTextEdit* releaseNotesEdit = new QPlainTextEdit(notes);
        releaseNotesEdit->setReadOnly(true);
        ui->releaseNotes->insertWidget(0, releaseNotesEdit);
    }

    reset();

}

void Installer::reset(const QString &message)
{
    state = STATE_IDLE;
    bytesDownloaded = 0;
    if (imageFile.isOpen()) {
        imageFile.close();
    }
    ui->deviceSelectBox->setEnabled(true);
    ui->releaseLinks->setEnabled(true);
    ui->linksButton->setEnabled(true);
    ui->downloadButton->setEnabled(true);
    ui->writeButton->setEnabled( !ui->fileNameLabel->text().isEmpty());
    ui->loadButton->setEnabled(true);
    ui->hdmiOutputButton->setEnabled(true);
    ui->sdtvOutputButton->setEnabled(false);
    ui->cancelButton->setEnabled(false);
    ui->refreshDeiceListButton->setEnabled(true);
    ui->removableDevicesComboBox->setEnabled(true);
    isCancelled = false;
    ui->messageBar->setText(message);

}

void Installer::disableControls()
{
    ui->deviceSelectBox->setEnabled(false);
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

    qDebug() << selectedVersion;

    checksum = checksumMap[selectedVersion];
    qDebug() << checksum;

    if (checksum.isEmpty() || downloadSum != checksum.toUtf8()) {
        qDebug() << "checksum:" << checksum.toUtf8();
        qDebug() << "downloadsum:" << downloadSum;
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
    case STATE_GET_SUPPORTED_DEVICES:
        parseAndSetSupportedDevices(data);
        break;

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
        ui->messageBar->setText("Download complete, verifying checksum...");




        if (isChecksumValid()) {
            reset("Download complete, verifying checksum... Done!");
        }
        else {
            reset("Wrong md5 sum! Please re-download.");
        }
    }
}

void Installer::updateDevices()
{
    state = STATE_GET_SUPPORTED_DEVICES;
    disableControls();

    ui->messageBar->setText("Getting supported RasPlex devices");
    QUrl url("http://"+m_serverUrl+"/devices");
    manager->get(url);
}

void Installer::getDeviceReleases(int index)
{
    state = STATE_GETTING_LINKS;
    disableControls();


#if defined( Q_OS_WIN)
    QString PLATFORM = "windows";
#elif defined(Q_OS_LINUX)
    QString PLATFORM = "linux";
#elif defined(Q_OS_MAC)
    QString PLATFORM = "osx";
#endif

    QString deviceName = ui->deviceSelectBox->itemText(index);
    QString deviceId = ui->deviceSelectBox->itemData(index).toString();
    ui->messageBar->setText("Getting releases for "+deviceName);
    QUrl url("http://"+m_serverUrl+"/install?platform="+PLATFORM+"&device="+deviceId);
    manager->get(url);
}

void Installer::downloadImage()
{
    state = STATE_DOWNLOADING_IMAGE;
    disableControls();
    selectedVersion = ui->releaseLinks->itemText(ui->releaseLinks->currentIndex());
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

        qDebug() << QDir::homePath() + "/" + newFileName;

        QString savedir = QFileDialog::getExistingDirectory(this, tr("Directory To Store Image"),
                                                    QDir::homePath(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);


        newFileName = savedir +QDir::separator ()+ newFileName + ".img.gz";

        if (savedir.isEmpty() || newFileName.isEmpty()) {
            reset();
            return;
        }
        qDebug() << "Downloading to" << newFileName;

        if (imageFile.isOpen())
            imageFile.close();

        setImageFileName(newFileName);

        if (!imageFile.open(QFile::WriteOnly | QFile::Truncate)) {
            reset("Failed to open file for writing!");
            return;
        }

        ui->messageBar->setText("Downloading image... please be patient.");
        ui->cancelButton->setEnabled(true);
        imageHash.reset();
    }
    manager->get(url);
}

void Installer::getImageFileNameFromUser()
{

    QString filename = QFileDialog::getOpenFileName(this, "Open rasplex image", QDir::homePath());

    if (!filename.endsWith(".img.gz"))
    {
        QMessageBox::information(this, tr(" "), "You must select a .img.gz file.");
        return;
    }


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

    if (destination.isNull()) {
        reset();
        return;
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(getUncompressedImageSize());
    ui->messageBar->setText("Writing image to "+destination);

    // DiskWriter will re-open the image file.
    if (imageFile.isOpen()) {
        imageFile.close();
    }

    state = STATE_WRITING_IMAGE;
    emit proceedToWriteImageToDevice(imageFileName, destination);


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
/*
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
    */
    ui->messageBar->setText("Settings changed.");
    configHandler->unMount();
}

void Installer::writingFinished()
{
    // Make sure the config is updated
    selectVideoOutput();

    reset("Writing done!");
}

void Installer::writingSyncing()
{
    if (state == STATE_WRITING_IMAGE)
        ui->messageBar->setText("Syncing file system.");
}
