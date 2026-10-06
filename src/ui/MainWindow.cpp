#include "MainWindow.h"
#include "../common/Theme.h"
#include "../common/Utils.h"
#include "../core/DriveManager.h"
#include "ConflictDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QStatusBar>
#include <QMessageBox>
#include <QStyle>
#include <QSet>
#include <QFileInfo>
#include <QDir>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    m_manager = new TransferManager(this);
    m_scanner = new SizeScanner(this);

    setupUi();
    applyTheme();

    connect(m_manager, &TransferManager::itemAdded, this, &MainWindow::onItemAdded);
    connect(m_manager, &TransferManager::itemUpdated, this, &MainWindow::onItemUpdated);
    connect(m_manager, &TransferManager::itemCompleted, this, &MainWindow::onItemCompleted);
    connect(m_manager, &TransferManager::allCompleted, this, &MainWindow::onAllCompleted);

    updateSummary();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("FluxTransfer"));
    resize(1320, 860);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(12);

    auto* header = new QHBoxLayout();
    auto* titles = new QVBoxLayout();
    titles->setSpacing(2);

    auto* title = new QLabel(QStringLiteral("FluxTransfer"), this);
    title->setObjectName("titleLabel");

    m_summaryLabel = new QLabel(QStringLiteral("Ready"), this);
    m_summaryLabel->setObjectName("subtitleLabel");

    titles->addWidget(title);
    titles->addWidget(m_summaryLabel);

    auto* modeGroup = new QGroupBox(QStringLiteral("Mode"), this);
    auto* modeLayout = new QHBoxLayout(modeGroup);
    modeLayout->setContentsMargins(10, 4, 10, 6);

    m_copyMode = new QRadioButton(QStringLiteral("Copy"), this);
    m_moveMode = new QRadioButton(QStringLiteral("Move"), this);
    m_copyMode->setChecked(true);
    m_copyMode->setToolTip(QStringLiteral("Leave the originals in place"));
    m_moveMode->setToolTip(QStringLiteral("Delete the originals after a verified transfer"));

    modeLayout->addWidget(m_copyMode);
    modeLayout->addWidget(m_moveMode);

    header->addLayout(titles, 1);
    header->addWidget(modeGroup);
    root->addLayout(header);

    m_warningLabel = new QLabel(this);
    m_warningLabel->setObjectName("warningLabel");
    m_warningLabel->setWordWrap(true);
    m_warningLabel->setVisible(false);
    root->addWidget(m_warningLabel);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* sourceCard = new QFrame(this);
    sourceCard->setObjectName("cardFrame");
    auto* sourceLayout = new QVBoxLayout(sourceCard);
    sourceLayout->setContentsMargins(12, 10, 12, 12);
    sourceLayout->setSpacing(8);

    auto* sourceHeader = new QHBoxLayout();
    auto* sourceTitle = new QLabel(QStringLiteral("Source locations"), sourceCard);
    sourceTitle->setObjectName("subtitleLabel");

    m_checklist = new SourceChecklist(m_scanner, sourceCard);

    sourceHeader->addWidget(sourceTitle);
    sourceHeader->addStretch();
    sourceLayout->addLayout(sourceHeader);
    sourceLayout->addWidget(m_checklist, 1);

    auto* destCard = new QFrame(this);
    destCard->setObjectName("cardFrame");
    auto* destLayout = new QVBoxLayout(destCard);
    destLayout->setContentsMargins(12, 10, 12, 12);
    destLayout->setSpacing(10);

    auto* destTitle = new QLabel(QStringLiteral("Destination"), destCard);
    destTitle->setObjectName("subtitleLabel");

    m_destination = new DestinationPicker(destCard);

    m_workerCombo = new QComboBox(this);
    m_workerCombo->addItem(QStringLiteral("1 worker"), 1);
    m_workerCombo->addItem(QStringLiteral("2 workers"), 2);
    m_workerCombo->addItem(QStringLiteral("4 workers"), 4);
    m_workerCombo->addItem(QStringLiteral("8 workers"), 8);
    m_workerCombo->setCurrentIndex(2);

    m_verifyCheckBox = new QCheckBox(QStringLiteral("Verify after transfer (byte compare)"), this);

    m_startBtn = new QPushButton(QStringLiteral("Start Transfer"), this);
    m_startBtn->setObjectName("primaryButton");
    m_startBtn->setMinimumHeight(38);

    auto* threadsRow = new QHBoxLayout();
    threadsRow->addWidget(new QLabel(QStringLiteral("Threads:"), this));
    threadsRow->addWidget(m_workerCombo);
    threadsRow->addStretch(1);

    auto* actionRow = new QHBoxLayout();
    actionRow->addWidget(m_startBtn);

    destLayout->addWidget(destTitle);
    destLayout->addWidget(m_destination);
    destLayout->addSpacing(4);
    destLayout->addLayout(threadsRow);
    destLayout->addWidget(m_verifyCheckBox);
    destLayout->addStretch(1);
    destLayout->addLayout(actionRow);

    splitter->addWidget(sourceCard);
    splitter->addWidget(destCard);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({ 760, 520 });

    root->addWidget(splitter, 3);

    m_queueWidget = new TransferQueueWidget(this);
    root->addWidget(m_queueWidget, 2);

    statusBar()->showMessage(QStringLiteral("Ready"));

    connect(m_checklist, &SourceChecklist::sourcesChanged, this, &MainWindow::updateSummary);
    connect(m_destination, &DestinationPicker::destinationChanged,
            this, [this] { updateSummary(); });
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartTransfer);
    connect(m_workerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                m_manager->setMaxConcurrent(m_workerCombo->currentData().toInt());
            });
    connect(m_queueWidget, &TransferQueueWidget::cancelRequested, this, &MainWindow::onCancelAll);
    connect(m_queueWidget, &TransferQueueWidget::retryRequested,
            m_manager, &TransferManager::retry);
}

void MainWindow::applyTheme()
{
    setStyleSheet(Theme::instance().stylesheet());
}

void MainWindow::updateSummary()
{
    const QList<SourceEntry> sources = m_checklist->checkedSources();
    const QString dest = m_destination->destination();

    if (sources.isEmpty() && dest.isEmpty()) {
        m_summaryLabel->setText(QStringLiteral("Ready"));
        setWarning(QString(), false);
        return;
    }

    const qint64 bytes = m_checklist->checkedBytes();
    const QString size = (m_checklist->anySizeUnknown() ? QStringLiteral("~") : QString())
                         + DriveManager::formatSize(bytes);

    m_summaryLabel->setText(QString("%1 source%2 · %3 → %4")
                                .arg(sources.size())
                                .arg(sources.size() == 1 ? QString() : QStringLiteral("s"))
                                .arg(size, dest.isEmpty() ? QStringLiteral("(no destination)")
                                                         : QFileInfo(dest).fileName()));

    if (sources.isEmpty()) {
        setWarning(QStringLiteral("Select at least one source location."), true);
        return;
    }

    if (!m_destination->isValid()) {
        setWarning(QStringLiteral("Choose a destination directory."), true);
        return;
    }

    const qint64 free = m_destination->freeBytes();
    if (free >= 0 && !m_checklist->anySizeUnknown() && bytes > free) {
        setWarning(QStringLiteral("Not enough space: %1 needed but only %2 free at the destination.")
                       .arg(DriveManager::formatSize(bytes), DriveManager::formatSize(free)),
                   true);
        return;
    }

    if (m_moveMode->isChecked()) {
        setWarning(QStringLiteral("Move mode deletes each source folder after it transfers successfully."),
                   false);
        return;
    }

    setWarning(QString(), false);
}

void MainWindow::setWarning(const QString& message, bool blocking)
{
    m_warningLabel->setVisible(!message.isEmpty());
    m_warningLabel->setText(message);
    m_warningLabel->setProperty("blocking", blocking);
    m_warningLabel->style()->unpolish(m_warningLabel);
    m_warningLabel->style()->polish(m_warningLabel);
}

bool MainWindow::validateSelection(QString& reason) const
{
    const QList<SourceEntry> sources = m_checklist->checkedSources();
    if (sources.isEmpty()) {
        reason = QStringLiteral("Select at least one source location.");
        return false;
    }

    if (!m_destination->isValid()) {
        reason = QStringLiteral("Choose a destination directory.");
        return false;
    }

    const qint64 needed = m_checklist->checkedBytes();
    const qint64 free = m_destination->freeBytes();

    if (free >= 0 && !m_checklist->anySizeUnknown() && needed > free) {
        reason = QStringLiteral("Not enough free space at the destination.\n\nNeed %1, have %2.")
                     .arg(DriveManager::formatSize(needed), DriveManager::formatSize(free));
        return false;
    }

    return true;
}

bool MainWindow::resolveNameCollisions(QList<TransferItem>& items)
{
    const QString dest = m_destination->destination();

    QSet<QString> seen;
    QStringList duplicates;

    for (const TransferItem& item : items) {
        const QString key = item.displayName.toLower();
        if (seen.contains(key))
            duplicates.append(item.displayName);
        seen.insert(key);
    }

    if (duplicates.isEmpty())
        return true;

    ConflictDialog dialog(QStringLiteral("%1, %2")
                              .arg(duplicates.first(),
                                   duplicates.size() > 1
                                       ? QStringLiteral("and %1 more").arg(duplicates.size() - 1)
                                       : QString()),
                          this);
    dialog.setWindowTitle(QStringLiteral("Duplicate destination names"));

    if (dialog.exec() != QDialog::Accepted)
        return false;

    const int action = dialog.action();
    if (action == 0)
        return false;

    if (action == 2) {
        for (TransferItem& item : items) {
            const QFileInfo existing(QDir(dest).filePath(item.displayName));
            if (existing.exists()) {
                const QString base = existing.completeBaseName();
                const QString suffix = existing.suffix();
                int n = 1;
                QString candidate;
                do {
                    const QString numbered = suffix.isEmpty()
                        ? QStringLiteral("%1 (%2)").arg(base).arg(n)
                        : QStringLiteral("%1 (%2).%3").arg(base).arg(n).arg(suffix);
                    candidate = QDir(dest).filePath(numbered);
                    ++n;
                } while (QFileInfo::exists(candidate));
                item.displayName = QFileInfo(candidate).fileName();
            }
        }
    }

    return true;
}

QList<TransferItem> MainWindow::buildTransferItems()
{
    QList<TransferItem> items;

    const QString dest = m_destination->destination();
    const TransferMode mode = m_moveMode->isChecked() ? TransferMode::Move : TransferMode::Copy;
    const bool verify = m_verifyCheckBox->isChecked();

    for (const SourceEntry& source : m_checklist->checkedSources()) {
        TransferItem item(source.path,
                          QDir(dest).filePath(source.name),
                          source.name,
                          source.bytes,
                          mode,
                          verify);
        items.append(item);
    }

    return items;
}

void MainWindow::onStartTransfer()
{
    QString reason;
    if (!validateSelection(reason)) {
        QMessageBox::warning(this, QStringLiteral("Cannot start"), reason);
        return;
    }

    QList<TransferItem> items = buildTransferItems();

    if (!resolveNameCollisions(items))
        return;

    m_manager->enqueueBatch(items);

    statusBar()->showMessage(QStringLiteral("Transferring %1 item%2…")
                                 .arg(items.size())
                                 .arg(items.size() == 1 ? QString() : QStringLiteral("s")), 0);
}

void MainWindow::onItemAdded(const TransferItem& item)
{
    m_queueWidget->addItem(item);
}

void MainWindow::onItemUpdated(const TransferItem& item)
{
    m_queueWidget->updateItem(item);
}

void MainWindow::onItemCompleted(const QUuid& id, bool success, const QString& error)
{
    Q_UNUSED(id)

    if (!success)
        statusBar()->showMessage(QStringLiteral("Failed: %1").arg(error), 8000);
}

void MainWindow::onAllCompleted()
{
    statusBar()->showMessage(QStringLiteral("All transfers finished"), 5000);
    m_summaryLabel->setText(QStringLiteral("Done"));
}

void MainWindow::onCancelAll()
{
    m_manager->cancelAll();
    statusBar()->showMessage(QStringLiteral("Cancelling…"), 4000);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_manager->activeCount() > 0) {
        const auto reply = QMessageBox::question(
            this,
            QStringLiteral("Transfers in progress"),
            QStringLiteral("Active transfers will be cancelled. Quit anyway?"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }

        m_manager->cancelAll();
    }

    event->accept();
}