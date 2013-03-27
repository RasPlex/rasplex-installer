#ifndef INSTALLER_H
#define INSTALLER_H

#include <QDialog>
#include <QtXml>
#include <QNetworkAccessManager>

class DiskWriter;

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
    QNetworkAccessManager manager;

    void parseAndSetLinks(const QByteArray &data);
    void saveAndUpdateProgress(QNetworkReply *reply);
    void extractByteOffsetsFromContentLength(qlonglong &first, qlonglong &last, qlonglong &total, QString s);
    void reset();
    void disableControls();

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
        STATE_GETTING_LINKS,
        STATE_GETTING_URL,
        STATE_DOWNLOADING_IMAGE,
        STATE_WRITING_IMAGE
    } state;

    qlonglong bytesDownloaded;
    QString imageFileName;
    QFile imageFile;
    QUrl downloadUrl;
    QStringList devices;
    DiskWriter *diskWriter;
    bool isCancelled;

private slots:
    void cancel();
    void updateLinks();
    void getDownloadLink();
    void refreshDeviceList();
    void downloadImage(QNetworkReply *reply);
    void fileListReply(QNetworkReply *reply);
    void getImageFileNameFromUser();
    void writeImageToDevice();
};

#endif // INSTALLER_H
