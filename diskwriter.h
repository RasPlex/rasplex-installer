#ifndef DISKWRITER_H
#define DISKWRITER_H

#include <QString>
#include <QObject>

#ifdef Q_OS_UNIX
#include <QFile>
#elif defined(Q_OS_WIN32)
#include <QFile>
// TODO
#endif

class DiskWriter : public QObject
{
    Q_OBJECT

public:
    DiskWriter(QObject *parent = 0);
    ~DiskWriter();

    int open(QString device);
    void close();
    bool isOpen();
    bool writeCompressedImageToRemovableDevice(QString filename);

signals:
    void bytesWritten(int);

private:
    // Device data (fd's, handles, etc)
#ifdef Q_OS_UNIX
    QFile dev;
#elif defined(Q_OS_WIN)
    QFile dev;
    // TODO
#endif
};

#endif // DISKWRITER_H
