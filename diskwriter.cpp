#include "diskwriter.h"

#include "zlib.h"

#include <QCoreApplication>

void DiskWriter::cancelWrite()
{
    isCancelled = true;
}

bool DiskWriter::writeCompressedImageToRemovableDevice(const QString &filename, const QString &device)
{
    int r;
    bool ok;
    // 512 == common sector size
    char buf[512*1024];
    isCancelled = false;

    if (!open(device)) {
        emit error("Couldn't open " + device);
        return false;
    }

    // Open source
    gzFile src = gzopen(filename.toStdString().c_str(), "rb");
    if (src == NULL) {
        emit error("Couldn't open " + filename);
        this->close();
        return false;
    }

    if (gzbuffer(src, 128*1024) != 0) {
        emit error("Failed to set gz buffer size");
        gzclose_r(src);
        this->close();
        return false;
    }

    r = gzread(src, buf, sizeof(buf));
    while (r > 0 && ! isCancelled) {
        // TODO: Sanity check
        ok = this->write(buf, r);
        if (!ok) {
            emit error("Failed to write to " + device + "!");
            gzclose(src);
            this->close();
            return false;
        }
        emit bytesWritten(gztell(src));
        // Needed for cancelWrite() to work
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        r = gzread(src, buf, sizeof(buf));
    }

    if (r < 0) {
        emit error("Failed to read from " + filename + "!");
        gzclose_r(src);
        this->close();
        return false;
    }

    emit syncing();
    gzclose_r(src);
    this->sync();
    this->close();

    if (!isCancelled) {
        emit finished();
    }
    isCancelled = false;
    return true;
}


