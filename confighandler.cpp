#include "confighandler.h"
#include <QFile>
#include <QDebug>

void ConfigHandler::setConfigFilename(const QString &filename)
{
    this->filename = filename;
}

QString ConfigHandler::configFilename() const
{
    return this->filename;
}

bool ConfigHandler::changeSetting(const char *setting, const int value) const
{
    bool found = false;
    QFile file(filename);
    char buf[1024];
    QByteArray line;
    qint64 len, offset;

    if (!file.open(QFile::ReadWrite)) {
        qDebug() << "Failed to open" << filename;
        return false;
    }

    while (!found && !file.atEnd()) {
        // Read next line into memory
        len = file.readLine(buf, sizeof(buf));
        if (len <= 0) break;
        line = QByteArray(buf);

        if (line.startsWith(setting)) {
            found = true;
            offset = file.pos() - len;
        }
    }

    if (found) {
        QByteArray restOfFile;

        // Modify line
        int idx = line.indexOf('=');
        line = line.left(idx+1);
        line.append(QByteArray::number(value) + "\n");

        // Store rest of file in memory (potentially dangerous!)
        restOfFile.append(line);
        while (!file.atEnd()) {
            // Read next line into memory
            len = file.read(buf, sizeof(buf));
            if (len < 0) {
                found = false;
                break;
            }
            restOfFile.append(buf, len);
        }

        // Restore file with modified line
        file.seek(offset);
        len = file.write(restOfFile);
        if (len != restOfFile.size()) {
            qDebug() << "Wrote" << len << " bytes. Should have been" << restOfFile.size();
            found = false;
        }
    }
    // Append setting
    else {
        file.seek(file.size());
        line = QByteArray(setting);
        line += "=";
        line += QByteArray::number(value);
        line += "\n";
        file.write(line);
        found = true;
    }

    file.close();
    return found;
}
