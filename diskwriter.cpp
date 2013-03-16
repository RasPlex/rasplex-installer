#include "diskwriter.h"

#ifdef Q_OS_UNIX
DiskWriter::DiskWriter()
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

int DiskWriter::write(const char *buf, size_t n)
{
    if (!dev.isOpen()) {
        return -1;
    }

    return dev.write(buf, n);
}

#elif Q_OS_WIN32
    // TODO
#endif
