#include "SizeScanner.h"
#include <QDirIterator>
#include <QRunnable>
#include <QThreadPool>
#include <QPointer>
#include <QMutex>
#include <QDir>

namespace {

class SizeTask : public QRunnable
{
public:
    SizeTask(QPointer<SizeScanner> owner, QString path)
        : m_owner(std::move(owner)), m_path(std::move(path))
    {
        setAutoDelete(true);
    }

    void run() override
    {
        const qint64 bytes = SizeScanner::measure(m_path);

        if (m_owner.isNull())
            return;

        QMetaObject::invokeMethod(m_owner.data(), "onSizeComputed",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, m_path),
                                  Q_ARG(qint64, bytes));
    }

private:
    QPointer<SizeScanner> m_owner;
    QString m_path;
};

}

SizeScanner::SizeScanner(QObject* parent) : QObject(parent)
{
}

SizeScanner::~SizeScanner()
{
    invalidate();
    QThreadPool::globalInstance()->waitForDone(3000);
}

qint64 SizeScanner::cachedSize(const QString& path) const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.value(path, -1);
}

bool SizeScanner::isKnown(const QString& path) const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.contains(path);
}

void SizeScanner::request(const QString& path)
{
    if (path.isEmpty())
        return;

    {
        QMutexLocker locker(&m_mutex);
        if (m_cache.contains(path) || m_pending.contains(path))
            return;
        m_pending.insert(path);
    }

    QThreadPool::globalInstance()->start(new SizeTask(QPointer<SizeScanner>(this), path));
}

void SizeScanner::invalidate()
{
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
}

void SizeScanner::onSizeComputed(const QString& path, qint64 bytes)
{
    {
        QMutexLocker locker(&m_mutex);
        m_pending.remove(path);
        m_cache.insert(path, bytes);
    }

    emit sizeReady(path, bytes);
}

qint64 SizeScanner::measure(const QString& path)
{
    QDirIterator it(path,
                    QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);

    qint64 bytes = 0;

    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        if (info.isSymLink())
            continue;
        bytes += info.size();
    }

    return bytes;
}