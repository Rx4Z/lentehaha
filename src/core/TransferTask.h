#pragma once

#include <QString>
#include <QMetaType>
#include <QUuid>

enum class TransferStatus {
    Pending,
    InProgress,
    Completed,
    Failed,
    Cancelled,
    Paused
};

enum class TransferMode {
    Copy,
    Move
};

struct TransferItem
{
    QUuid id;
    QString sourcePath;
    QString destPath;
    QString displayName;
    qint64 totalBytes = 0;
    qint64 bytesTransferred = 0;
    double speedBps = 0.0;
    int percent = 0;
    TransferStatus status = TransferStatus::Pending;
    TransferMode mode = TransferMode::Copy;
    bool verify = false;
    QString errorMessage;

    TransferItem() = default;
    TransferItem(const QString& src, const QString& dst, const QString& name,
                 qint64 size, TransferMode m, bool verifyAfter = false)
        : id(QUuid::createUuid()), sourcePath(src), destPath(dst), displayName(name),
          totalBytes(size), mode(m), verify(verifyAfter) {}
};

Q_DECLARE_METATYPE(TransferItem)
Q_DECLARE_METATYPE(TransferStatus)
Q_DECLARE_METATYPE(TransferMode)
