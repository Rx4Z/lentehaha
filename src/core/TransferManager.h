#pragma once

#include <QObject>
#include <QThreadPool>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QHash>
#include <QVector>
#include <QPair>
#include <QElapsedTimer>
#include <QUuid>
#include "TransferTask.h"
#include "FileCopyEngine.h"

class TransferWorker;

class TransferManager : public QObject
{
    Q_OBJECT

public:
    explicit TransferManager(QObject* parent = nullptr);
    ~TransferManager() override;

    void enqueue(const TransferItem& item);
    void enqueueBatch(const QList<TransferItem>& items);
    void pause(const QUuid& id);
    void resume(const QUuid& id);
    void cancel(const QUuid& id);
    void cancelAll();
    void retry(const QUuid& id);

    QList<TransferItem> items() const;
    TransferItem item(const QUuid& id) const;
    int activeCount() const;
    int pendingCount() const;
    int completedCount() const;

    void setMaxConcurrent(int max);
    qint64 calculateDirectorySize(const QString& path);

signals:
    void itemAdded(const TransferItem& item);
    void itemUpdated(const TransferItem& item);
    void itemRemoved(const QUuid& id);
    void itemStarted(const QUuid& id);
    void itemVerifying(const QUuid& id);
    void itemProgress(const QUuid& id, qint64 bytesTransferred, qint64 totalBytes, double speedBps);
    void itemCompleted(const QUuid& id, bool success, const QString& error);
    void allCompleted();

private slots:
    void onWorkerFinished(const QUuid& id, bool success, const QString& error);

private:
    void startNext();
    bool isPaused(const QUuid& id) const;
    double sampleSpeed(const QUuid& id, qint64 bytes);

    mutable QMutex m_mutex;
    QQueue<TransferItem> m_queue;
    QHash<QUuid, TransferItem> m_items;
    QHash<QUuid, bool> m_paused;
    QThreadPool m_threadPool;
    FileCopyEngine m_engine;
    int m_maxConcurrent = 4;
    int m_activeCount = 0;

    QElapsedTimer m_clock;
    QHash<QUuid, QVector<QPair<qint64, qint64>>> m_speedSamples;
    QHash<QUuid, double> m_lastSpeed;
};
