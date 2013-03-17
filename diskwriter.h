#ifndef DISKWRITER_H
#define DISKWRITER_H

#include <QString>
#ifdef Q_OS_UNIX
#include <QFile>
#elif defined(Q_OS_WIN32)
// TODO
#endif

class DiskWriter
{
public:
    DiskWriter();
    ~DiskWriter();

    int open(QString device);
    void close();
    bool isOpen();
    int write(const char *buf, size_t n);

private:
    // Device data (fd's, handles, etc)
#ifdef Q_OS_UNIX
    QFile dev;
#elif defined(Q_OS_WIN32)
    // TODO
#endif
};

#endif // DISKWRITER_H
