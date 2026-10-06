#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include "../core/TransferTask.h"

class TransferQueueWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TransferQueueWidget(QWidget* parent = nullptr);

    void addItem(const TransferItem& item);
    void updateItem(const TransferItem& item);
    void removeItem(const QUuid& id);
    void clearCompleted();

signals:
    void cancelRequested(const QUuid& id);
    void retryRequested(const QUuid& id);
    void pauseRequested(const QUuid& id);
    void resumeRequested(const QUuid& id);
    void clearCompletedRequested();

private slots:
    void onCancelAll();
    void onClearCompleted();
    void onRetryFailed();

private:
    QTableWidget* m_table;
    QLabel* m_summaryLabel;
    QPushButton* m_cancelAllBtn;
    QPushButton* m_clearCompletedBtn;
    QPushButton* m_retryFailedBtn;

    void updateSummary();
    QString statusText(TransferStatus status) const;
};
