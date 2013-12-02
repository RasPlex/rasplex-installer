#include "diskwriter_unix.h"

#include <QMessageBox>
#include <QDebug>
#include <unistd.h>

#ifdef Q_OS_MAC
#include <QProcess>
#endif


DiskWriter_unix::DiskWriter_unix(QObject *parent) :
    DiskWriter(parent)
{
    isCancelled = false;
}

DiskWriter_unix::~DiskWriter_unix()
{
    if (isOpen()) {
        close();
    }
}

bool DiskWriter_unix::open(const QString& device)
{
    dev.setFileName(device);
#ifdef Q_OS_MAC
    // Write to RAW device, this is MUCH faster
    QString rawdev(device);
    rawdev.replace(QString("/dev/"),QString("/dev/r"));

    /* Unmount the device */
    QProcess unmount;
    unmount.start("diskutil unmountDisk "+rawdev, QIODevice::ReadOnly);
    unmount.waitForStarted();
    unmount.waitForFinished();
    qDebug() << unmount.readAll();
    dev.setFileName(rawdev);
#endif

    return dev.open(QFile::WriteOnly);
}

void DiskWriter_unix::close()
{
    dev.close();
}

void DiskWriter_unix::sync()
{
    if (this->isOpen()) {
        fsync(dev.handle());
    }
}

bool DiskWriter_unix::isOpen()
{
    return dev.isOpen();
}

bool DiskWriter_unix::write(const char *data, qint64 size)
{

    bool rc;
    rc = dev.write(data, size);

    if (rc)
       QMessageBox::information(NULL, tr(" "), "Image written successfully!");
    else
       QMessageBox::information(NULL, tr(" "), "Image failed to write : (");
    return rc;
}

