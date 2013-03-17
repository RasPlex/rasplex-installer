#include "diskwriter.h"
#include "zlib.h"

#include <QDebug>
#include <QApplication>

#if defined(Q_OS_UNIX)
DiskWriter::DiskWriter(QObject *parent) :
    QObject(parent)
{
}

DiskWriter::~DiskWriter()
{
    if (dev.isOpen()) {
        dev.close();
    }
}

int DiskWriter::open(QString device)
{
    dev.setFileName(device);
    if (!dev.open(QFile::WriteOnly)) {
        return -1;
    }
    return 0;
}

void DiskWriter::close()
{
    dev.close();
}

bool DiskWriter::isOpen()
{
    return dev.isOpen();
}

bool DiskWriter::writeCompressedImageToRemovableDevice(QString filename)
{
    int r;
    bool ok;
    // 512 == common sector size
    char buf[512*1024];

    if (!dev.isOpen()) {
        qDebug() << "Device not ready or whatever";
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
        return false;
    }

    r = gzread(src, buf, sizeof(buf));
    while (r > 0) {
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

    if (r < 0) {
        qDebug() << "Error reading file!";
        return false;
    }
    gzclose_r(src);
    return true;
}

#elif defined(Q_OS_WIN)
DiskWriter::DiskWriter(QObject *parent) :
    QObject(parent)
{
}

DiskWriter::~DiskWriter()
{
    if (dev.isOpen()) {
        dev.close();
    }
}

int DiskWriter::open(QString device)
{
    dev.setFileName(device);
    if (!dev.open(QFile::WriteOnly)) {
        return -1;
    }
    return 0;
}

void DiskWriter::close()
{
    dev.close();
}

bool DiskWriter::isOpen()
{
    return dev.isOpen();
}

bool DiskWriter::writeCompressedImageToRemovableDevice(QString filename)
{
    int r;
    bool ok;
    // 512 == common sector size
    char buf[512*1024];

    if (!dev.isOpen()) {
        qDebug() << "Device not ready or whatever";
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
        return false;
    }

    r = gzread(src, buf, sizeof(buf));
    while (r > 0) {
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

    if (r < 0) {
        qDebug() << "Error reading file!";
        return false;
    }
    gzclose_r(src);
    return true;
}
#endif
