#pragma once

#include <QString>
#include <QStringList>
#include <QFile>
#include <functional>
#include <atomic>

struct CopyProgress {
    qint64 bytesTransferred = 0;
    qint64 totalBytes = 0;
    double speedBps = 0.0;
    int percent = 0;
};

enum class CopyResult {
    Success,
    Cancelled,
    ErrorSourceNotFound,
    ErrorDestNotFound,
    ErrorPermissionDenied,
    ErrorDiskFull,
    ErrorUnknown
};

class FileCopyEngine
{
public:
    using ProgressCallback = std::function<void(const CopyProgress&)>;
    using CancelCallback = std::function<bool()>;

    FileCopyEngine();
    ~FileCopyEngine();

    CopyResult copyFile(const QString& source, const QString& dest,
                        ProgressCallback onProgress = nullptr,
                        CancelCallback shouldCancel = nullptr);

    CopyResult copyDirectory(const QString& sourceDir, const QString& destDir,
                             ProgressCallback onProgress = nullptr,
                             CancelCallback shouldCancel = nullptr);

    qint64 calculateDirectorySize(const QString& path);
    QStringList listFilesRecursive(const QString& path);

    bool verifyFile(const QString& source, const QString& dest);
    bool verifyDirectory(const QString& sourceDir, const QString& destDir);

    void cancel();
    bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

private:
    CopyResult copyFileInternal(const QString& source, const QString& dest,
                                ProgressCallback onProgress,
                                CancelCallback shouldCancel,
                                qint64& totalBytes);
    CopyResult copyDirectoryInternal(const QString& sourceDir, const QString& destDir,
                                     ProgressCallback onProgress,
                                     CancelCallback shouldCancel,
                                     qint64& bytesCopied,
                                     qint64 totalBytes);

    std::atomic<bool> m_cancelled{false};
    static constexpr qint64 BUFFER_SIZE = 1024 * 1024;
};
