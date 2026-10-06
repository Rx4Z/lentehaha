#include "TransferManager.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

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

        auto progressCb = [this](const CopyProgress& p) {
            emit progress(m_id, p.bytesTransferred, p.totalBytes, p.speedBps);
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
            default: error = success ? QString() : "Unknown error"; break;
        }

        if (success && m_item.verify) {
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

        emit finished(m_id, success, error);
    }

signals:
    void started(const QUuid& id);
    void progress(const QUuid& id, qint64 bytesTransferred, qint64 totalBytes, double speedBps);
    void finished(const QUuid& id, bool success, const QString& error);

private:
    QUuid m_id;
    TransferItem m_item;
    FileCopyEngine* m_engine;
    TransferManager* m_manager;
};

#include "TransferManager.moc"

TransferManager::TransferManager(QObject* parent) : QObject(parent)
{
    m_threadPool.setMaxThreadCount(m_maxConcurrent);
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
            it.value().status == TransferStatus::InProgress) {
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
            emit itemStarted(id);
        });
        connect(worker, &TransferWorker::progress, this, [this](const QUuid& id, qint64 bytes, qint64 total, double speed) {
            QMutexLocker l(&m_mutex);
            if (m_items.contains(id)) {
                TransferItem item = m_items[id];
                item.bytesTransferred = bytes;
                item.totalBytes = total;
                item.speedBps = speed;
                item.percent = total > 0 ? static_cast<int>((bytes * 100) / total) : 0;
                m_items[id] = item;
                l.unlock();
                emit itemUpdated(item);
            }
            emit itemProgress(id, bytes, total, speed);
        });
        connect(worker, &TransferWorker::finished, this, &TransferManager::onWorkerFinished);

        m_threadPool.start(worker);

        locker.relock();
    }
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
        m_items[id] = item;
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
