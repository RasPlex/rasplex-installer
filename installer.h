#ifndef INSTALLER_H
#define INSTALLER_H

#include <QCryptographicHash>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QtXml>

#include "downloadmanager.h"

class QThread;
class DiskWriter;
class DeviceEnumerator;
class ConfigHandler;

namespace Ui {
class Installer;
}

class Installer : public QDialog
{
    Q_OBJECT
    
public:
    explicit Installer(QWidget *parent = 0);
    ~Installer();
    
private:
    Ui::Installer *ui;

    QXmlSimpleReader xmlReader;
    DownloadManager* manager;

    void parseAndSetLinks(const QByteArray &data);
    void saveAndUpdateProgress(QNetworkReply *reply);
    void disableControls();
    bool isChecksumValid();

    QByteArray rangeByteArray(qlonglong first, qlonglong last);
    QNetworkRequest createRequest(QUrl &url, qlonglong first, qlonglong last);
    unsigned int getUncompressedImageSize();
    void setImageFileName(QString filename);

    enum {
        RESPONSE_OK = 200,
        RESPONSE_PARTIAL = 206,
        RESPONSE_FOUND = 302,
        RESPONSE_REDIRECT = 307,
        RESPONSE_BAD_REQUEST = 400,
        RESPONSE_NOT_FOUND = 404
    };
    enum {
        STATE_IDLE,
        STATE_GET_SUPPORTED_DEVICES,
        STATE_GETTING_LINKS,
        STATE_GETTING_URL,
        STATE_DOWNLOADING_IMAGE,
        STATE_WRITING_IMAGE
    } state;

    qlonglong bytesDownloaded;
    QString imageFileName;
    QCryptographicHash imageHash;
    QFile imageFile;
    QString downloadUrl;
    QString checksum;
    QString selectedVersion;
    QString selectedSaveDir;
    QMap<QString, QString> checksumMap;
    DiskWriter *diskWriter;
    QThread* diskWriterThread;
    DeviceEnumerator* devEnumerator;
    ConfigHandler *configHandler;
    bool isCancelled;
    static const QString m_serverUrl;

signals:
    void proceedToWriteImageToDevice(const QString& image, const QString& device);

private slots:
    void handleFinishedDownload(const QByteArray& data);
    void handlePartialData(const QByteArray& data, qlonglong total);
    void cancel();
    void updateDevices();
    void parseAndSetSupportedDevices(const QByteArray &data);
    void getDeviceReleases(int index);
    void refreshRemovablesList();
    void downloadImage();
    void getImageFileNameFromUser();
    void writeImageToDevice();
    void selectVideoOutput();
    void writingFinished();
    void writingSyncing();
    void reset(const QString& message = "Please download and select an image to write.");
};

#endif // INSTALLER_H
