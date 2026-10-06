#include "TransferQueueWidget.h"
#include "../common/Theme.h"
#include "../common/Utils.h"
#include "../core/DriveManager.h"
#include <QDateTime>

TransferQueueWidget::TransferQueueWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* headerLayout = new QHBoxLayout();
    m_summaryLabel = new QLabel("No transfers", this);
    m_summaryLabel->setObjectName("subtitleLabel");

    m_cancelAllBtn = new QPushButton("Cancel All", this);
    m_clearCompletedBtn = new QPushButton("Clear Completed", this);
    m_retryFailedBtn = new QPushButton("Retry Failed", this);
    m_retryFailedBtn->setObjectName("primaryButton");

    m_cancelAllBtn->setEnabled(false);
    m_clearCompletedBtn->setEnabled(false);
    m_retryFailedBtn->setEnabled(false);

    headerLayout->addWidget(m_summaryLabel, 1);
    headerLayout->addWidget(m_retryFailedBtn);
    headerLayout->addWidget(m_clearCompletedBtn);
    headerLayout->addWidget(m_cancelAllBtn);

    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({"Name", "Status", "Progress", "Size", "Speed", "ETA"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(false);

    layout->addLayout(headerLayout);
    layout->addWidget(m_table, 1);

    connect(m_cancelAllBtn, &QPushButton::clicked, this, &TransferQueueWidget::onCancelAll);
    connect(m_clearCompletedBtn, &QPushButton::clicked, this, &TransferQueueWidget::onClearCompleted);
    connect(m_retryFailedBtn, &QPushButton::clicked, this, &TransferQueueWidget::onRetryFailed);
}

void TransferQueueWidget::addItem(const TransferItem& item)
{
    int row = m_table->rowCount();
    m_table->insertRow(row);

    m_table->setItem(row, 0, new QTableWidgetItem(item.displayName));
    m_table->setItem(row, 1, new QTableWidgetItem(statusText(item.status)));
    m_table->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(item.percent)));
    m_table->setItem(row, 3, new QTableWidgetItem(DriveManager::formatSize(item.totalBytes)));
    m_table->setItem(row, 4, new QTableWidgetItem(item.speedBps > 0 ? Utils::formatSpeed(item.speedBps) : "-"));
    m_table->setItem(row, 5, new QTableWidgetItem("-"));

    m_table->item(row, 0)->setData(Qt::UserRole, item.id.toString());
    updateSummary();
}

void TransferQueueWidget::updateItem(const TransferItem& item)
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toString() == item.id.toString()) {
            m_table->item(row, 1)->setText(statusText(item.status));
            m_table->item(row, 2)->setText(QString("%1%").arg(item.percent));
            m_table->item(row, 3)->setText(DriveManager::formatSize(item.totalBytes));
            m_table->item(row, 4)->setText(item.speedBps > 0 ? Utils::formatSpeed(item.speedBps) : "-");

            if (item.status == TransferStatus::InProgress && item.speedBps > 0 && item.totalBytes > item.bytesTransferred) {
                double remaining = (item.totalBytes - item.bytesTransferred) / item.speedBps;
                m_table->item(row, 5)->setText(Utils::formatEta(remaining));
            } else {
                m_table->item(row, 5)->setText("-");
            }

            if (item.status == TransferStatus::Completed) {
                m_table->item(row, 1)->setForeground(Theme::instance().success());
            } else if (item.status == TransferStatus::Failed) {
                m_table->item(row, 1)->setForeground(Theme::instance().error());
            } else if (item.status == TransferStatus::InProgress) {
                m_table->item(row, 1)->setForeground(Theme::instance().warning());
            }
            break;
        }
    }
    updateSummary();
}

void TransferQueueWidget::removeItem(const QUuid& id)
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toString() == id.toString()) {
            m_table->removeRow(row);
            break;
        }
    }
    updateSummary();
}

void TransferQueueWidget::clearCompleted()
{
    for (int row = m_table->rowCount() - 1; row >= 0; --row) {
        QString status = m_table->item(row, 1)->text();
        if (status == "Completed" || status == "Failed" || status == "Cancelled") {
            m_table->removeRow(row);
        }
    }
    updateSummary();
}

void TransferQueueWidget::updateSummary()
{
    int total = m_table->rowCount();
    int completed = 0;
    int failed = 0;
    int inProgress = 0;

    for (int row = 0; row < total; ++row) {
        QString status = m_table->item(row, 1)->text();
        if (status == "Completed") completed++;
        else if (status == "Failed") failed++;
        else if (status == "In Progress") inProgress++;
    }

    m_summaryLabel->setText(QString("%1 items • %2 active • %3 completed • %4 failed")
        .arg(total).arg(inProgress).arg(completed).arg(failed));

    m_cancelAllBtn->setEnabled(inProgress > 0);
    m_clearCompletedBtn->setEnabled(completed > 0 || failed > 0);
    m_retryFailedBtn->setEnabled(failed > 0);
}

QString TransferQueueWidget::statusText(TransferStatus status) const
{
    switch (status) {
        case TransferStatus::Pending: return "Pending";
        case TransferStatus::InProgress: return "In Progress";
        case TransferStatus::Completed: return "Completed";
        case TransferStatus::Failed: return "Failed";
        case TransferStatus::Cancelled: return "Cancelled";
        case TransferStatus::Paused: return "Paused";
    }
    return "Unknown";
}

void TransferQueueWidget::onCancelAll()
{
    emit cancelRequested(QUuid());
}

void TransferQueueWidget::onClearCompleted()
{
    clearCompleted();
}

void TransferQueueWidget::onRetryFailed()
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 1)->text() == "Failed") {
            QUuid id = QUuid(m_table->item(row, 0)->data(Qt::UserRole).toString());
            emit retryRequested(id);
        }
    }
}
