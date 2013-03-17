#ifndef DISKWRITER_WINDOWS_H
#define DISKWRITER_WINDOWS_H

#include "diskwriter.h"
#include <QFile>

class DiskWriter_windows : public DiskWriter
{
    Q_OBJECT
public:
    explicit DiskWriter_windows(QObject *parent = 0);
    ~DiskWriter_windows();

    int open(const QString &device);
    void close();
    bool isOpen();
    bool writeCompressedImageToRemovableDevice(const QString &filename);

private:
    QFile dev;
};

#endif // DISKWRITER_WINDOWS_H
