#ifndef DISKWRITER_UNIX_H
#define DISKWRITER_UNIX_H

#include "diskwriter.h"
#include <QFile>

class DiskWriter_unix : public DiskWriter
{
    Q_OBJECT
public:
    explicit DiskWriter_unix(QObject *parent = 0);
    ~DiskWriter_unix();


public slots:
    bool writeCompressedImageToRemovableDevice(const QString &filename, const QString& device);

private:
    QFile dev;

    bool open(const QString &device);
    void close();
    void sync();
    bool isOpen();
    bool write(const char *data, qint64 size);
};

#endif // DISKWRITER_UNIX_H
