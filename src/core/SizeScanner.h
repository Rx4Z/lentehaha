#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QMutex>

class SizeScanner : public QObject
{
    Q_OBJECT

public:
    explicit SizeScanner(QObject* parent = nullptr);
    ~SizeScanner() override;

    qint64 cachedSize(const QString& path) const;
    bool isKnown(const QString& path) const;

    void request(const QString& path);
    void invalidate();

    static qint64 measure(const QString& path);

signals:
    void sizeReady(const QString& path, qint64 bytes);

private slots:
    void onSizeComputed(const QString& path, qint64 bytes);

private:
    mutable QMutex m_mutex;
    QHash<QString, qint64> m_cache;
    QSet<QString> m_pending;
};