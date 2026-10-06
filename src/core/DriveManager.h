#pragma once

#include <QString>
#include <QStringList>
#include <QList>

struct DriveInfo
{
    QString letter;
    QString label;
    QString path;
    qint64 totalBytes = 0;
    qint64 freeBytes = 0;
    bool isReady = false;
    bool isRemovable = false;
    bool isNetwork = false;
    bool isCDROM = false;

    QString kindLabel() const;
};

struct KnownFolder
{
    QString name;
    QString path;
};

class DriveManager
{
public:
    // includeSystemDrive defaults to false: the Windows system drive is not
    // offered as a bulk source, since copying from it is rarely intended.
    static QList<DriveInfo> enumerateDrives(bool includeSystemDrive = false);
    static QString systemDriveLetter();
    static QString homeDirectory();
    static QList<KnownFolder> knownFolders();
    static QStringList topLevelFolders(const QString& drivePath);
    static bool pathExists(const QString& path);
    static qint64 freeSpace(const QString& path);
    static qint64 totalSpace(const QString& path);
    static QString formatSize(qint64 bytes);
};