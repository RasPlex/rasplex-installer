#include "diskwriter.h"

#include "zlib.h"
#include <stdlib.h>

#include <QCoreApplication>

void DiskWriter::cancelWrite()
{
    isCancelled = true;
}

bool DiskWriter::writeCompressedImageToRemovableDevice(const QString &filename, const QString &device)
{
    int r;
    bool ok;
    unsigned int BUFFSIZE=512*1024*sizeof(char); // must malloc or it'll segfault on osx... can't handle stack properly, fucking apple
    char* buf;

    buf = (char*) malloc(BUFFSIZE);
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

    if (gzbuffer(src, BUFFSIZE) != 0) {
        emit error("Failed to set gz buffer size");
        gzclose_r(src);
        this->close();
        return false;
    }

    r = gzread(src, buf, BUFFSIZE);
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
        r = gzread(src, buf, 8192);
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


