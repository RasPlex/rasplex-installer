#include "diskwriter.h"

#include "zlib.h"

#include <QCoreApplication>

void DiskWriter::cancelWrite()
{
    isCancelled = true;
}

void DiskWriter::writeCompressedImageToRemovableDevice(const QString &filename, const QString &device)
{
    int r;
    bool ok;
    QByteArray buf(512*1024*sizeof(char), 0);

    isCancelled = false;

    if (!open(device)) {
        emit error("Couldn't open " + device);
        return;
    }

    // Open source
    gzFile src = gzopen(filename.toStdString().c_str(), "rb");
    if (src == NULL) {
        emit error("Couldn't open " + filename);
        this->close();
        return;
    }

    if (gzbuffer(src, buf.size()) != 0) {
        emit error("Failed to set gz buffer size");
        gzclose_r(src);
        this->close();
        return;
    }

    r = gzread(src, buf.data(), buf.size());
    while (r > 0 && ! isCancelled) {
        // TODO: Sanity check
        ok = this->write(buf);
        if (!ok) {
            emit error("Failed to write to " + device + "!");
            gzclose(src);
            this->close();
            return;
        }
        this->sync();
        emit bytesWritten(gztell(src));
        r = gzread(src, buf.data(), buf.size());
    }

    if (r < 0) {
        emit error("Failed to read from " + filename + "!");
        gzclose_r(src);
        this->close();
        return;
    }

    emit syncing();
    gzclose_r(src);
    this->sync();
    this->close();

    if (isCancelled) {
        emit bytesWritten(0);
    }
    else {
        emit finished();
    }
    isCancelled = false;
}


