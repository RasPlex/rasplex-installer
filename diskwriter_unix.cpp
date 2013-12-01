#include "diskwriter_unix.h"

#include "zlib.h"

#include <QDebug>
#include <QApplication>

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

#ifdef Q_OS_MAC
    // Write to RAW device, this is MUCH faster
    device.replace("/dev/","/dev/r");

    /* Unmount the device */
    QProcess unmount;
    unmount.start("diskutil unmountDisk "+device, QIODevice::ReadOnly);
    unmount.waitForStarted();
    unmount.waitForFinished();
    qDebug() << unmount.readAll();
#endif

    dev.setFileName(device);
    return dev.open(QFile::WriteOnly);
}

void DiskWriter_unix::close()
{
    dev.close();
}

bool DiskWriter_unix::isOpen()
{
    return dev.isOpen();
}

void DiskWriter_unix::cancelWrite()
{
    isCancelled = true;
}

bool DiskWriter_unix::writeCompressedImageToRemovableDevice(const QString &filename, const QString &device)
{
    int r;
    bool ok;
    // 512 == common sector size
    char buf[512*1024];
    isCancelled = false;

    if (!open(device)) {
        qDebug() << "Could not open" << device;
        return false;
    }

    // Open source
    gzFile src = gzopen(filename.toStdString().c_str(), "rb");
    if (src == NULL) {
        qDebug() << "Couldn't open file:" << filename;
        return false;
    }

    if (gzbuffer(src, 128*1024) != 0) {
        qDebug() << "Failed to set buffer size";
        gzclose_r(src);
        return false;
    }

    r = gzread(src, buf, sizeof(buf));
    while (r > 0 && ! isCancelled) {
        // TODO: Sanity check
        ok = dev.write(buf, r);
        if (!ok) {
            qDebug() << "Error writing";
            return false;
        }
        emit bytesWritten(gztell(src));
        QApplication::processEvents();
        r = gzread(src, buf, sizeof(buf));
    }

    isCancelled = false;
    if (r < 0) {
        gzclose_r(src);
        qDebug() << "Error reading file!";
        return false;
    }

    dev.close();
    gzclose_r(src);
    return true;
}

