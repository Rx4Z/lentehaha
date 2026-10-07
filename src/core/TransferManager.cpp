#include "TransferManager.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <atomic>

namespace {
// Minimun gap between progress emissions per transfer: keeps UI updates to
// roughly 10 Hz instead of one per 4 MB chunk (hundreds per second on fast
// disks), which saturated the queued-signal pump on big transfers.
constexpr qint64 kProgressEmitGapMs = 100;

// Rolling speed window: the manager derives speed from bytes transferred over
// the last 3s, so the number is a current rate instead of a lifetime average.
constexpr qint64 kSpeedWindowMs = 3000;
}

class TransferWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    TransferWorker(QUuid id, TransferItem item, FileCopyEngine* engine, TransferManager* manager)
        : m_id(id), m_item(item), m_engine(engine), m_manager(manager)
    {
        setAutoDelete(true);
    }

    void run() override
    {
        emit started(m_id);

        // Single source of truth for progress. Engine callbacks land on this
        // worker thread; the atomics keep the running total monotonic and the
        // throttle below caps how often the GUI is told.
        m_bytes.store(0, std::memory_order_relaxed);
        m_total.store(0, std::memory_order_relaxed);

        QElapsedTimer throttle;
        throttle.start();

        auto progressCb = [this, &throttle](const CopyProgress& p) {
            m_bytes.store(p.bytesTransferred, std::memory_order_relaxed);
            m_total.store(p.totalBytes, std::memory_order_relaxed);

            // First signal always goes out so the UI can paint immediately;
            // afterwards emit at most every kProgressEmitGapMs.
            const qint64 elapsed = throttle.elapsed();
            if (elapsed >= kProgressEmitGapMs) {
                emit progress(m_id, m_bytes.load(std::memory_order_relaxed),
                              m_total.load(std::memory_order_relaxed));
                throttle.restart();
            }
        };

        auto cancelCb = [this]() {
            return m_engine->isCancelled();
        };

        CopyResult result;
        QFileInfo srcInfo(m_item.sourcePath);

        if (srcInfo.isDir()) {
            result = m_engine->copyDirectory(m_item.sourcePath, m_item.destPath, progressCb, cancelCb);
        } else {
            result = m_engine->copyFile(m_item.sourcePath, m_item.destPath, progressCb, cancelCb);
        }

        bool success = (result == CopyResult::Success);
        QString error;
        switch (result) {
            case CopyResult::Cancelled: error = "Cancelled"; break;
            case CopyResult::ErrorSourceNotFound: error = "Source not found"; break;
            case CopyResult::ErrorPermissionDenied: error = "Permission denied"; break;
            case CopyResult::ErrorDiskFull: error = "Disk full"; break;
            case CopyResult::ErrorOverlappingPaths:
                error = "Source and destination overlap"; break;
            default: error = success ? QString() : "Unknown error"; break;
        }

        if (success && m_item.verify) {
            // Announce the byte-compare phase so the UI can paint a
            // "Verifying" status instead of sitting on a frozen 100%
            // In Progress row while the destination is re-read.
            emit verifying(m_id);

            const bool verified = srcInfo.isDir()
                ? m_engine->verifyDirectory(m_item.sourcePath, m_item.destPath)
                : m_engine->verifyFile(m_item.sourcePath, m_item.destPath);

            if (!verified) {
                success = false;
                error = "Verification failed (destination does not match source)";
            }
        }

        if (m_item.mode == TransferMode::Move && success) {
            if (srcInfo.isDir()) {
                QDir(m_item.sourcePath).removeRecursively();
            } else {
                QFile::remove(m_item.sourcePath);
            }
        }

        // Force a final, unthrottled update so the last chunk (and empty
        // files, which never hit the throttle) still report 100% before the
        // worker tears down, even if the emit gap has not elapsed.
        if (m_total.load(std::memory_order_relaxed) > 0 ||
            m_bytes.load(std::memory_order_relaxed) > 0) {
            emit progress(m_id, m_bytes.load(std::memory_order_relaxed),
                          m_total.load(std::memory_order_relaxed));
        }

        emit finished(m_id, success, error);
    }

signals:
    void started(const QUuid& id);
    void progress(const QUuid& id, qint64 bytesTransferred, qint64 totalBytes);
    void verifying(const QUuid& id);
    void finished(const QUuid& id, bool success, const QString& error);

private:
    QUuid m_id;
    TransferItem m_item;
    FileCopyEngine* m_engine;
    TransferManager* m_manager;
    std::atomic<qint64> m_bytes{0};
    std::atomic<qint64> m_total{0};
};

#include "TransferManager.moc"

TransferManager::TransferManager(QObject* parent) : QObject(parent)
{
    m_threadPool.setMaxThreadCount(m_maxConcurrent);
    m_clock.start();
}

TransferManager::~TransferManager()
{
    cancelAll();
    m_threadPool.waitForDone(5000);
}

void TransferManager::enqueue(const TransferItem& item)
{
    QMutexLocker locker(&m_mutex);
    m_items[item.id] = item;
    m_queue.enqueue(item);
    locker.unlock();

    emit itemAdded(item);
    startNext();
}

void TransferManager::enqueueBatch(const QList<TransferItem>& items)
{
    QMutexLocker locker(&m_mutex);
    for (const auto& item : items) {
        m_items[item.id] = item;
        m_queue.enqueue(item);
        emit itemAdded(item);
    }
    locker.unlock();

    startNext();
}

void TransferManager::pause(const QUuid& id)
{
    QMutexLocker locker(&m_mutex);
    m_paused[id] = true;
    if (m_items.contains(id)) {
        TransferItem item = m_items[id];
        item.status = TransferStatus::Paused;
        m_items[id] = item;
        emit itemUpdated(item);
    }
}

void TransferManager::resume(const QUuid& id)
{
    QMutexLocker locker(&m_mutex);
    m_paused[id] = false;
    if (m_items.contains(id)) {
        TransferItem item = m_items[id];
        item.status = TransferStatus::Pending;
        m_items[id] = item;
        m_queue.enqueue(item);
        emit itemUpdated(item);
    }
    locker.unlock();

    startNext();
}

void TransferManager::cancel(const QUuid& id)
{
    QMutexLocker locker(&m_mutex);
    m_paused.remove(id);
    if (m_items.contains(id)) {
        TransferItem item = m_items[id];
        item.status = TransferStatus::Cancelled;
        m_items[id] = item;
        emit itemUpdated(item);
    }
}

void TransferManager::cancelAll()
{
    m_engine.cancel();
    QMutexLocker locker(&m_mutex);
    for (auto it = m_items.begin(); it != m_items.end(); ++it) {
        if (it.value().status == TransferStatus::Pending ||
            it.value().status == TransferStatus::InProgress ||
            it.value().status == TransferStatus::Verifying) {
            it.value().status = TransferStatus::Cancelled;
            emit itemUpdated(it.value());
        }
    }
}

void TransferManager::retry(const QUuid& id)
{
    QMutexLocker locker(&m_mutex);
    if (m_items.contains(id)) {
        TransferItem item = m_items[id];
        item.status = TransferStatus::Pending;
        item.bytesTransferred = 0;
        item.percent = 0;
        item.speedBps = 0;
        item.errorMessage.clear();
        m_items[id] = item;
        m_speedSamples.remove(id);
        m_lastSpeed.remove(id);
        m_queue.enqueue(item);
        emit itemUpdated(item);
    }
    locker.unlock();

    startNext();
}

QList<TransferItem> TransferManager::items() const
{
    QMutexLocker locker(&m_mutex);
    return m_items.values();
}

TransferItem TransferManager::item(const QUuid& id) const
{
    QMutexLocker locker(&m_mutex);
    return m_items.value(id);
}

int TransferManager::activeCount() const
{
    return m_activeCount;
}

int TransferManager::pendingCount() const
{
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (const auto& item : m_items) {
        if (item.status == TransferStatus::Pending) count++;
    }
    return count;
}

int TransferManager::completedCount() const
{
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (const auto& item : m_items) {
        if (item.status == TransferStatus::Completed ||
            item.status == TransferStatus::Failed ||
            item.status == TransferStatus::Cancelled) count++;
    }
    return count;
}

void TransferManager::setMaxConcurrent(int max)
{
    m_maxConcurrent = max;
    m_threadPool.setMaxThreadCount(max);
}

qint64 TransferManager::calculateDirectorySize(const QString& path)
{
    return m_engine.calculateDirectorySize(path);
}

void TransferManager::startNext()
{
    QMutexLocker locker(&m_mutex);

    while (m_activeCount < m_maxConcurrent && !m_queue.isEmpty()) {
        TransferItem item = m_queue.dequeue();

        if (m_paused.value(item.id, false)) {
            m_queue.enqueue(item);
            continue;
        }

        if (item.status != TransferStatus::Pending) continue;

        item.status = TransferStatus::InProgress;
        m_items[item.id] = item;
        m_activeCount++;

        locker.unlock();

        TransferWorker* worker = new TransferWorker(item.id, item, &m_engine, this);
        connect(worker, &TransferWorker::started, this, [this](const QUuid& id) {
            // Seed the speed window so the first sample has a reference point
            // instead of deriving an infinite rate from a single datapoint.
            m_speedSamples[id].clear();
            m_speedSamples[id].append({m_clock.elapsed(), 0});
            m_lastSpeed[id] = 0.0;
            emit itemStarted(id);
        });
        connect(worker, &TransferWorker::progress, this, [this](const QUuid& id, qint64 bytes, qint64 total) {
            const double speed = sampleSpeed(id, bytes);
            QMutexLocker l(&m_mutex);
            if (m_items.contains(id)) {
                TransferItem item = m_items[id];
                item.bytesTransferred = bytes;
                item.totalBytes = total;
                item.speedBps = speed;
                item.percent = total > 0 ? qBound(0, static_cast<int>((bytes * 100) / total), 100) : 0;
                m_items[id] = item;
                l.unlock();
                emit itemUpdated(item);
            }
            emit itemProgress(id, bytes, total, speed);
        });
        connect(worker, &TransferWorker::verifying, this,
                [this](const QUuid& id) {
                    QMutexLocker l(&m_mutex);
                    if (m_items.contains(id)) {
                        TransferItem item = m_items[id];
                        item.status = TransferStatus::Verifying;
                        item.verifyState = VerifyState::Verifying;
                        m_items[id] = item;
                        l.unlock();
                        emit itemUpdated(item);
                    }
                    emit itemVerifying(id);
                });
        connect(worker, &TransferWorker::finished, this, &TransferManager::onWorkerFinished);

        m_threadPool.start(worker);

        locker.relock();
    }
}

double TransferManager::sampleSpeed(const QUuid& id, qint64 bytes)
{
    const auto now = m_clock.elapsed();

    // A worker restart (retry) rewinds bytes; the old window no longer holds,
    // so drop it and start over from zero again.
    if (!m_speedSamples.contains(id) ||
        (!m_speedSamples[id].isEmpty() && bytes < m_speedSamples[id].last().second)) {
        m_speedSamples[id].clear();
        m_speedSamples[id].append({now, 0});
        m_lastSpeed.remove(id);
    }

    QVector<QPair<qint64, qint64>>& samples = m_speedSamples[id];
    samples.append({now, bytes});

    // Trim samples older than the rolling window so the ratio stays current.
    const qint64 horizon = now - kSpeedWindowMs;
    while (samples.size() > 2 && samples.first().first < horizon)
        samples.removeFirst();

    const qint64 dt = samples.last().first - samples.first().first;
    const qint64 db = samples.last().second - samples.first().second;

    double speed = 0.0;
    if (dt > 0 && db > 0)
        speed = static_cast<double>(db) * 1000.0 / dt;

    // Keep the last meaningful rate as a fallback during quiet windows, so the
    // UI never shows an ETA flicker down to zero between two chunks.
    if (speed > 0.0)
        m_lastSpeed[id] = speed;
    else
        speed = m_lastSpeed.value(id, 0.0);

    return speed;
}

void TransferManager::onWorkerFinished(const QUuid& id, bool success, const QString& error)
{
    QMutexLocker locker(&m_mutex);

    m_activeCount = qMax(0, m_activeCount - 1);

    if (m_items.contains(id)) {
        TransferItem item = m_items[id];
        item.status = success ? TransferStatus::Completed : TransferStatus::Failed;
        item.errorMessage = error;
        item.percent = success ? 100 : item.percent;
        if (item.verify) {
            // A completed byte-compare resolves the verify column. Only a
            // verified tree is Passed; a failed copy or a byte mismatch both
            // land on Failed (the mismatch text above says why).
            item.verifyState = success ? VerifyState::Passed : VerifyState::Failed;
        }
        m_items[id] = item;
        m_speedSamples.remove(id);
        m_lastSpeed.remove(id);
        locker.unlock();

        emit itemUpdated(item);
        emit itemCompleted(id, success, error);
    } else {
        locker.unlock();
    }

    startNext();

    if (m_activeCount == 0 && m_queue.isEmpty()) {
        emit allCompleted();
    }
}

bool TransferManager::isPaused(const QUuid& id) const
{
    QMutexLocker locker(&m_mutex);
    return m_paused.value(id, false);
}
