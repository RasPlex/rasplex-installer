#ifndef DISKWRITER_H
#define DISKWRITER_H

#include <QObject>
#include <QString>
#include <QStringList>

class DiskWriter : public QObject
{
    Q_OBJECT

public:
    DiskWriter(QObject *parent = 0) : QObject(parent) {}
    virtual ~DiskWriter() {}

    virtual int open(QString device) = 0;
    virtual void close() = 0;
    virtual bool isOpen() = 0;
    virtual bool writeCompressedImageToRemovableDevice(const QString &filename) = 0;
    virtual QStringList getRemovableDeviceNames() = 0;
    virtual void cancelWrite() = 0;
    virtual QStringList getUserFriendlyNames(QStringList devices) = 0;

signals:
    void bytesWritten(int);

protected:
    bool isCancelled;
};

#endif // DISKWRITER_H
