#include "FileCopyEngine.h"
#include "../common/Utils.h"
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>
#include <QDirIterator>
#include <QHash>
#include <windows.h>
#include <vector>
#include <cstring>

namespace {

// Every traversal (size, list, copy, verify) MUST use identical filters,
// otherwise verification reports files as "missing" purely because a
// traversal disagreed about which files exist. Hidden/system files such as
// desktop.ini count, so they must also be copied.
constexpr QDir::Filters kEntryFilters =
    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden;

constexpr QDir::Filters kIteratorFilters =
    QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden;

// Symlinks and directory junctions are skipped: following one can recurse
// out of the source tree or loop forever, and copying one would silently
// duplicate an entire unrelated tree.
bool isSkippableLink(const QFileInfo& info)
{
    return info.isSymLink();
}

} // namespace

FileCopyEngine::FileCopyEngine() {}

FileCopyEngine::~FileCopyEngine() {}

void FileCopyEngine::cancel()
{
    m_cancelled.store(true, std::memory_order_release);
}

CopyResult FileCopyEngine::copyFile(const QString& source, const QString& dest,
                                     ProgressCallback onProgress,
                                     CancelCallback shouldCancel)
{
    qint64 totalBytes = 0;
    return copyFileInternal(source, dest, onProgress, shouldCancel, totalBytes);
}

CopyResult FileCopyEngine::copyFileInternal(const QString& source, const QString& dest,
                                             ProgressCallback onProgress,
                                             CancelCallback shouldCancel,
                                             qint64& totalBytes)
{
    QFileInfo srcInfo(source);
    if (!srcInfo.exists()) return CopyResult::ErrorSourceNotFound;

    // Refuse before touching anything: opening dest with WriteOnly|Truncate
    // on the same path as the source zeroes the source in place (this is
    // what wiped files on E:). Overlapping paths are never valid here.
    if (Utils::pathsOverlap(source, dest))
        return CopyResult::ErrorOverlappingPaths;

    totalBytes = srcInfo.size();

    QDir().mkpath(QFileInfo(dest).absolutePath());

    QFile srcFile(source);
    if (!srcFile.open(QIODevice::ReadOnly)) return CopyResult::ErrorPermissionDenied;

    QFile dstFile(dest);
    if (!dstFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        srcFile.close();
        return CopyResult::ErrorPermissionDenied;
    }

    std::vector<char> buffer(BUFFER_SIZE);
    qint64 totalWritten = 0;
    QElapsedTimer timer;
    timer.start();

    while (!srcFile.atEnd()) {
        if (m_cancelled.load(std::memory_order_acquire) ||
            (shouldCancel && shouldCancel())) {
            srcFile.close();
            dstFile.close();
            dstFile.remove();
            return CopyResult::Cancelled;
        }

        qint64 bytesRead = srcFile.read(buffer.data(), BUFFER_SIZE);
        if (bytesRead <= 0) break;

        qint64 written = dstFile.write(buffer.data(), bytesRead);
        if (written <= 0) {
            srcFile.close();
            dstFile.close();
            return CopyResult::ErrorDiskFull;
        }

        totalWritten += written;

        if (onProgress) {
            CopyProgress p;
            p.bytesTransferred = totalWritten;
            p.totalBytes = totalBytes;
            p.percent = totalBytes > 0 ? static_cast<int>((totalWritten * 100) / totalBytes) : 0;
            qint64 elapsed = timer.elapsed();
            p.speedBps = elapsed > 0 ? (totalWritten * 1000.0) / elapsed : 0.0;
            onProgress(p);
        }
    }

    srcFile.close();
    dstFile.close();

    if (!dstFile.flush()) {
        return CopyResult::ErrorDiskFull;
    }

    return CopyResult::Success;
}

CopyResult FileCopyEngine::copyDirectory(const QString& sourceDir, const QString& destDir,
                                          ProgressCallback onProgress,
                                          CancelCallback shouldCancel)
{
    const qint64 totalBytes = calculateDirectorySize(sourceDir);
    qint64 bytesCopied = 0;
    return copyDirectoryInternal(sourceDir, destDir, onProgress, shouldCancel,
                                 bytesCopied, totalBytes);
}

CopyResult FileCopyEngine::copyDirectoryInternal(const QString& sourceDir, const QString& destDir,
                                                 ProgressCallback onProgress,
                                                 CancelCallback shouldCancel,
                                                 qint64& bytesCopied,
                                                 qint64 totalBytes)
{
    QDir srcDir(sourceDir);
    if (!srcDir.exists()) return CopyResult::ErrorSourceNotFound;

    // Refuse before mkpath: copying a directory onto itself truncates its
    // files in place, and a destination inside the source tree would make
    // the traversal recurse into the copy while it is being created.
    if (Utils::pathsOverlap(sourceDir, destDir))
        return CopyResult::ErrorOverlappingPaths;

    if (!QDir().mkpath(destDir)) return CopyResult::ErrorPermissionDenied;

    const QFileInfoList entries =
        srcDir.entryInfoList(kEntryFilters, QDir::Name);

    QElapsedTimer timer;
    timer.start();

    const auto emitAgg = [&](qint64 transferred) {
        if (!onProgress) return;
        CopyProgress agg;
        agg.bytesTransferred = transferred;
        agg.totalBytes = totalBytes;
        agg.percent = totalBytes > 0
            ? static_cast<int>((transferred * 100) / totalBytes) : 0;
        if (agg.percent > 100) agg.percent = 100;
        const qint64 elapsed = timer.elapsed();
        agg.speedBps = elapsed > 0 ? (transferred * 1000.0) / elapsed : 0.0;
        onProgress(agg);
    };

    for (const QFileInfo& entry : entries) {
        if (m_cancelled.load(std::memory_order_acquire) ||
            (shouldCancel && shouldCancel())) {
            return CopyResult::Cancelled;
        }

        const QString srcPath = entry.absoluteFilePath();
        const QString dstPath = QDir(destDir).filePath(entry.fileName());

        if (isSkippableLink(entry))
            continue;

        // Snapshot of what has already been copied so nested callbacks can
        // report an absolute aggregate instead of per-subtree totals.
        const qint64 base = bytesCopied;

        if (entry.isDir()) {
            qint64 subCopied = 0;
            const CopyResult r = copyDirectoryInternal(srcPath, dstPath,
                [&](const CopyProgress& p) { emitAgg(base + p.bytesTransferred); },
                shouldCancel, subCopied, totalBytes);
            if (r == CopyResult::Cancelled) return r;
            if (r != CopyResult::Success) return r;
            bytesCopied += subCopied;
            emitAgg(bytesCopied);
        } else {
            qint64 fileAccum = 0;
            const CopyResult r = copyFileInternal(srcPath, dstPath,
                [&](const CopyProgress& p) { emitAgg(base + p.bytesTransferred); },
                shouldCancel, fileAccum);
            if (r == CopyResult::Cancelled) return r;
            if (r != CopyResult::Success) return r;
            bytesCopied += fileAccum;
            emitAgg(bytesCopied);
        }
    }

    emitAgg(bytesCopied);
    return CopyResult::Success;
}

bool FileCopyEngine::verifyFile(const QString& source, const QString& dest)
{
    QFileInfo srcInfo(source);
    QFileInfo dstInfo(dest);

    if (!srcInfo.exists() || !dstInfo.exists())
        return false;

    if (srcInfo.size() != dstInfo.size())
        return false;

    if (srcInfo.size() == 0)
        return true;

    QFile srcFile(source);
    QFile dstFile(dest);

    if (!srcFile.open(QIODevice::ReadOnly))
        return false;
    if (!dstFile.open(QIODevice::ReadOnly))
        return false;

    std::vector<char> srcBuf(BUFFER_SIZE);
    std::vector<char> dstBuf(BUFFER_SIZE);

    while (!srcFile.atEnd()) {
        const qint64 read = srcFile.read(srcBuf.data(), BUFFER_SIZE);
        if (read <= 0)
            break;

        if (dstFile.read(dstBuf.data(), read) != read)
            return false;

        if (std::memcmp(srcBuf.data(), dstBuf.data(), static_cast<size_t>(read)) != 0)
            return false;
    }

    return true;
}

bool FileCopyEngine::verifyDirectory(const QString& sourceDir, const QString& destDir)
{
    QHash<QString, qint64> sourceMap;
    {
        QDirIterator it(sourceDir, kIteratorFilters, QDirIterator::Subdirectories);
        const QString root = QDir::fromNativeSeparators(sourceDir);
        while (it.hasNext()) {
            it.next();
            const QFileInfo info = it.fileInfo();
            if (isSkippableLink(info))
                continue;
            const QString full = QDir::fromNativeSeparators(info.absoluteFilePath());
            sourceMap.insert(full.mid(root.length()), info.size());
        }
    }

    QHash<QString, qint64> destMap;
    {
        QDirIterator it(destDir, kIteratorFilters, QDirIterator::Subdirectories);
        const QString root = QDir::fromNativeSeparators(destDir);
        while (it.hasNext()) {
            it.next();
            const QFileInfo info = it.fileInfo();
            if (isSkippableLink(info))
                continue;
            const QString full = QDir::fromNativeSeparators(info.absoluteFilePath());
            destMap.insert(full.mid(root.length()), info.size());
        }
    }

    if (sourceMap.size() != destMap.size())
        return false;

    for (auto it = sourceMap.constBegin(); it != sourceMap.constEnd(); ++it) {
        const auto found = destMap.constFind(it.key());
        if (found == destMap.constEnd() || found.value() != it.value())
            return false;
    }

    return true;
}

qint64 FileCopyEngine::calculateDirectorySize(const QString& path)
{
    qint64 total = 0;
    QDirIterator it(path, kIteratorFilters, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (isSkippableLink(it.fileInfo()))
            continue;
        total += it.fileInfo().size();
    }
    return total;
}

QStringList FileCopyEngine::listFilesRecursive(const QString& path)
{
    QStringList files;
    QDirIterator it(path, kIteratorFilters, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (isSkippableLink(it.fileInfo()))
            continue;
        files.append(it.fileInfo().absoluteFilePath());
    }
    return files;
}
