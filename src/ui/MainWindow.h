#pragma once

#include <QMainWindow>
#include <QRadioButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFrame>
#include <QCloseEvent>

#include "../core/TransferManager.h"
#include "../core/SizeScanner.h"
#include "SourceChecklist.h"
#include "DestinationPicker.h"
#include "TransferQueueWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onStartTransfer();
    void onItemAdded(const TransferItem& item);
    void onItemUpdated(const TransferItem& item);
    void onItemCompleted(const QUuid& id, bool success, const QString& error);
    void onAllCompleted();
    void onCancelAll();
    void updateSummary();

private:
    void setupUi();
    void applyTheme();
    QList<TransferItem> buildTransferItems();
    bool validateSelection(QString& reason) const;
    bool resolveNameCollisions(QList<TransferItem>& items);
    void setWarning(const QString& message, bool blocking);

    TransferManager* m_manager;
    SizeScanner* m_scanner;

    SourceChecklist* m_checklist;
    DestinationPicker* m_destination;

    QRadioButton* m_copyMode;
    QRadioButton* m_moveMode;
    QComboBox* m_workerCombo;
    QCheckBox* m_verifyCheckBox;
    QPushButton* m_startBtn;

    QLabel* m_summaryLabel;
    QLabel* m_warningLabel;
    TransferQueueWidget* m_queueWidget;
};