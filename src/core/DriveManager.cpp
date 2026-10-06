#include "DriveManager.h"
#include <QDir>
#include <QStorageInfo>
#include <QStandardPaths>
#include <QFileInfo>
#include <windows.h>

namespace {

UINT winDriveType(const QString& root)
{
    const std::wstring wroot = root.toStdWString();
    return GetDriveTypeW(wroot.c_str());
}

QStorageInfo storageForPath(const QString& path)
{
    QFileInfo info(path);
    QString root = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    return QStorageInfo(root);
}

}

QString DriveInfo::kindLabel() const
{
    if (isRemovable) return "Removable";
    if (isNetwork) return "Network";
    if (isCDROM) return "Optical";
    return "Fixed";
}

QString DriveManager::systemDriveLetter()
{
    wchar_t buffer[MAX_PATH] = {};
    const UINT written = GetWindowsDirectoryW(buffer, MAX_PATH);
    if (written == 0 || written >= MAX_PATH)
        return QString();

    // "C:\Windows" -> "C:"
    return QString::fromWCharArray(buffer, 2);
}

QList<DriveInfo> DriveManager::enumerateDrives(bool includeSystemDrive)
{
    QList<DriveInfo> drives;

    const QString systemLetter = includeSystemDrive ? QString() : systemDriveLetter();

    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady())
            continue;

        const QString root = storage.rootPath();
        const UINT type = winDriveType(root);

        if (type == DRIVE_CDROM)
            continue;

        DriveInfo info;
        info.path = QDir::fromNativeSeparators(root);
        info.letter = storage.displayName();
        while (info.letter.size() > 1 && info.letter.endsWith(QLatin1Char('/')))
            info.letter.chop(1);
        info.label = info.kindLabel();

        if (!systemLetter.isEmpty() &&
            info.letter.compare(systemLetter, Qt::CaseInsensitive) == 0) {
            continue;
        }

        info.totalBytes = storage.bytesTotal();
        info.freeBytes = storage.bytesFree();
        info.isReady = true;
        info.isRemovable = (type == DRIVE_REMOVABLE);
        info.isNetwork = (type == DRIVE_REMOTE);
        info.isCDROM = (type == DRIVE_CDROM);

        drives.append(info);
    }

    return drives;
}

QString DriveManager::homeDirectory()
{
    return QDir::fromNativeSeparators(QDir::homePath());
}

QList<KnownFolder> DriveManager::knownFolders()
{
    QList<KnownFolder> folders;

    struct Candidate
    {
        const char* label;
        QStandardPaths::StandardLocation location;
    };

    static const Candidate candidates[] = {
        { "Desktop",  QStandardPaths::DesktopLocation },
        { "Downloads", QStandardPaths::DownloadLocation },
        { "Documents", QStandardPaths::DocumentsLocation },
        { "Pictures",  QStandardPaths::PicturesLocation },
        { "Music",     QStandardPaths::MusicLocation },
        { "Videos",    QStandardPaths::MoviesLocation },
    };

    for (const Candidate& candidate : candidates) {
        const QStringList paths = QStandardPaths::standardLocations(candidate.location);
        for (const QString& path : paths) {
            if (!path.isEmpty() && QDir(path).exists()) {
                folders.append({ QString::fromLatin1(candidate.label),
                                 QDir::fromNativeSeparators(path) });
                break;
            }
        }
    }

    return folders;
}

QStringList DriveManager::topLevelFolders(const QString& drivePath)
{
    QStringList out;
    QDir dir(drivePath);

    const QFileInfoList entries =
        dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);

    for (const QFileInfo& entry : entries) {
        // Windows bookkeeping folders cannot be copied and would only produce
        // "permission denied" failures if offered as sources.
        const QString name = entry.fileName();
        if (name.compare(QStringLiteral("System Volume Information"), Qt::CaseInsensitive) == 0)
            continue;
        if (name.startsWith(QLatin1Char('$')))
            continue;

        out.append(QDir::fromNativeSeparators(entry.absoluteFilePath()));
    }

    return out;
}

bool DriveManager::pathExists(const QString& path)
{
    return QDir(path).exists();
}

qint64 DriveManager::freeSpace(const QString& path)
{
    return storageForPath(path).bytesFree();
}

qint64 DriveManager::totalSpace(const QString& path)
{
    return storageForPath(path).bytesTotal();
}

QString DriveManager::formatSize(qint64 bytes)
{
    static const QStringList units = { "B", "KB", "MB", "GB", "TB", "PB" };

    if (bytes < 0)
        return "--";

    double size = static_cast<double>(bytes);
    int unitIndex = 0;

    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        ++unitIndex;
    }

    return QString("%1 %2")
        .arg(size, 0, 'f', unitIndex > 0 ? 2 : 0)
        .arg(units.at(unitIndex));
}