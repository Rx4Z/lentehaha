#include "SourceChecklist.h"
#include "../core/SizeScanner.h"
#include "../common/Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QTimer>
#include <QtAlgorithms>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

namespace {
constexpr int kPathRole = Qt::UserRole;
constexpr int kKindRole = Qt::UserRole + 1;
constexpr int kBytesRole = Qt::UserRole + 2;
constexpr int kSizeTextRole = Qt::UserRole + 3;

// A checkable row reserves a check column in front of its text: a 15px
// QSS indicator (plus its 1px border on each side), a 2px margin-right and
// Qt's own check-to-text spacing. Rows without a check state (drive roots and
// the "Loading…" placeholder) get no such column, so their text lands 25px to
// the left of their checkable neighbours and the tree reads as two different
// columns. Shift those rows back into the shared column.
constexpr int kCheckColumnWidth = 25;

class AlignRowsWithoutCheckbox : public QStyledItemDelegate
{
public:
    explicit AlignRowsWithoutCheckbox(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        if (index.column() == 0 && !(option->features & QStyleOptionViewItem::HasCheckIndicator))
            option->rect.setLeft(option->rect.left() + kCheckColumnWidth);
    }
};

// Re-entrancy guard: nested scopes must not clobber each other, so use a
// counter that is always restored by RAII instead of a shared bool.
class Guard
{
public:
    explicit Guard(int& counter) : m_counter(counter) { ++m_counter; }
    ~Guard() { --m_counter; }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;

private:
    int& m_counter;
};
}

SourceChecklist::SourceChecklist(SizeScanner* scanner, QWidget* parent)
    : QWidget(parent), m_scanner(scanner)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({ "Location", "Size" });
    m_tree->setRootIsDecorated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setAlternatingRowColors(false);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->setItemDelegate(new AlignRowsWithoutCheckbox(m_tree));

    layout->addWidget(m_tree, 1);

    auto* footer = new QHBoxLayout();
    m_summary = new QLabel(this);
    m_summary->setObjectName("statusLabel");

    m_selectAllBtn = new QPushButton("Select all", this);
    m_clearBtn = new QPushButton("Clear", this);
    m_refreshBtn = new QPushButton("Refresh", this);

    footer->addWidget(m_summary, 1);
    footer->addWidget(m_selectAllBtn);
    footer->addWidget(m_clearBtn);
    footer->addWidget(m_refreshBtn);
    layout->addLayout(footer);

    connect(m_tree, &QTreeWidget::itemChanged, this, &SourceChecklist::onItemChanged);
    connect(m_tree, &QTreeWidget::itemExpanded, this, &SourceChecklist::onItemExpanded);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &SourceChecklist::onSelectAll);
    connect(m_clearBtn, &QPushButton::clicked, this, &SourceChecklist::onClearAll);
    connect(m_refreshBtn, &QPushButton::clicked, this, [this] { refresh(); });

    if (m_scanner)
        connect(m_scanner, &SizeScanner::sizeReady, this, &SourceChecklist::onSizeReady);

    refresh();
}

QTreeWidgetItem* SourceChecklist::buildHomeGroup()
{
    const QList<KnownFolder> folders = DriveManager::knownFolders();

    auto* homeItem = new QTreeWidgetItem(m_tree);
    homeItem->setText(0, QStringLiteral("Home"));
    homeItem->setText(1, QString());
    homeItem->setData(0, kPathRole, DriveManager::homeDirectory());
    homeItem->setData(0, kKindRole, KindHomeGroup);
    homeItem->setFlags(homeItem->flags() | Qt::ItemIsUserCheckable);
    homeItem->setForeground(0, Theme::instance().text());
    QFont f = homeItem->font(0);
    f.setBold(true);
    homeItem->setFont(0, f);
    homeItem->setExpanded(true);

    Guard guard(m_guard);
    for (const KnownFolder& folder : folders) {
        QTreeWidgetItem* child = buildFolderItem(folder.name, folder.path);
        homeItem->addChild(child);
        child->setCheckState(0, Qt::Checked);
    }

    refreshAncestors(homeItem);
    return homeItem;
}

QTreeWidgetItem* SourceChecklist::buildFolderItem(const QString& name, const QString& path)
{
    auto* item = new QTreeWidgetItem();
    item->setText(0, name);
    item->setData(0, kPathRole, path);
    item->setData(0, kKindRole, KindFolder);
    item->setData(0, kBytesRole, QVariant::fromValue<qint64>(-1));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

    const qint64 cached = m_scanner ? m_scanner->cachedSize(path) : -1;
    if (cached >= 0) {
        item->setData(0, kBytesRole, QVariant::fromValue<qint64>(cached));
        item->setText(1, DriveManager::formatSize(cached));
    } else {
        item->setText(1, QStringLiteral("scanning…"));
    }

    return item;
}

QTreeWidgetItem* SourceChecklist::buildDriveGroup(const DriveInfo& drive)
{
    auto* item = new QTreeWidgetItem(m_tree);
    item->setText(0, QString("%1  —  %2").arg(drive.letter, drive.label));
    item->setText(1, DriveManager::formatSize(drive.freeBytes) + QStringLiteral(" free"));
    item->setData(0, kPathRole, drive.path);
    item->setData(0, kKindRole, KindDriveGroup);
    item->setForeground(0, Theme::instance().text());
    item->setForeground(1, Theme::instance().textMuted());
    QFont f = item->font(0);
    f.setBold(true);
    item->setFont(0, f);

    // QTreeWidgetItem is checkable by DEFAULT. A drive row must never be
    // selectable as a source in its own right, otherwise the whole volume gets
    // copied instead of just the folders the user ticked. Expand-only.
    item->setFlags((item->flags() | Qt::ItemIsEnabled) & ~Qt::ItemIsUserCheckable);

    // A QTreeWidgetItem with no children draws NO expander arrow, so the row
    // looks inert and cannot be clicked open. A placeholder child gives Qt
    // something to draw an arrow for; it is swapped for real folders on expand.
    addPlaceholder(item, QStringLiteral("Loading…"));

    return item;
}

void SourceChecklist::addPlaceholder(QTreeWidgetItem* parent, const QString& text)
{
    auto* placeholder = new QTreeWidgetItem(parent);
    placeholder->setText(0, text);
    placeholder->setData(0, kKindRole, KindPlaceholder);
    placeholder->setFlags(Qt::ItemIsEnabled);
    placeholder->setForeground(0, Theme::instance().textMuted());
}

void SourceChecklist::populateDriveChildren(QTreeWidgetItem* driveItem)
{
    if (!driveItem)
        return;

    // Already populated with real folders?
    if (driveItem->childCount() > 0 &&
        driveItem->child(0)->data(0, kKindRole).toInt() != KindPlaceholder) {
        return;
    }

    const QString root = driveItem->data(0, kPathRole).toString();

    Guard guard(m_guard);

    // Detach and free the placeholder. Safe because callers reach this only
    // from a queued event (see onItemExpanded), never from inside the view's
    // own expand handling, where freeing a child is a use-after-free.
    while (driveItem->childCount() > 0)
        delete driveItem->takeChild(0);

    const QStringList folders = DriveManager::topLevelFolders(root);

    if (folders.isEmpty()) {
        addPlaceholder(driveItem, QStringLiteral("No folders found"));
        return;
    }

    for (const QString& folder : folders)
        driveItem->addChild(buildFolderItem(QFileInfo(folder).fileName(), folder));
}

void SourceChecklist::populateAllDrives()
{
    const QList<DriveInfo> drives = DriveManager::enumerateDrives();
    for (const DriveInfo& drive : drives)
        buildDriveGroup(drive);
}

void SourceChecklist::refresh()
{
    Guard guard(m_guard);

    m_tree->clear();
    QTreeWidgetItem* home = buildHomeGroup();
    populateAllDrives();

    // Re-assert the intended defaults once the tree is fully attached so the
    // initial state cannot depend on signal ordering during construction.
    setChildrenChecked(home, Qt::Checked);
    refreshAncestors(home);

    requestSizes();
    updateSummary();
    emit sourcesChanged();
}

void SourceChecklist::requestSizes()
{
    if (!m_scanner)
        return;

    // Walk every real folder, including lazily-created drive children,
    // otherwise their Size column would stay on "scanning…" forever.
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        QTreeWidgetItem* item = *it;
        if (item->data(0, kKindRole).toInt() == KindFolder)
            m_scanner->request(item->data(0, kPathRole).toString());
        ++it;
    }
}

void SourceChecklist::setChildrenChecked(QTreeWidgetItem* parent, Qt::CheckState state)
{
    if (!parent)
        return;

    Guard guard(m_guard);
    for (int i = 0; i < parent->childCount(); ++i) {
        QTreeWidgetItem* child = parent->child(i);
        if (child->flags().testFlag(Qt::ItemIsUserCheckable))
            child->setCheckState(0, state);
    }
}

void SourceChecklist::refreshAncestors(QTreeWidgetItem* item)
{
    Guard guard(m_guard);

    while (item) {
        if (item->flags().testFlag(Qt::ItemIsUserCheckable)) {
            int checked = 0;
            int total = 0;

            for (int i = 0; i < item->childCount(); ++i) {
                QTreeWidgetItem* child = item->child(i);
                if (!child->flags().testFlag(Qt::ItemIsUserCheckable))
                    continue;
                ++total;
                if (child->checkState(0) == Qt::Checked)
                    ++checked;
            }

            Qt::CheckState state = Qt::Unchecked;
            if (total > 0) {
                if (checked == total)
                    state = Qt::Checked;
                else if (checked > 0)
                    state = Qt::PartiallyChecked;
            }

            item->setCheckState(0, state);
        }
        item = item->parent();
    }
}

void SourceChecklist::onItemChanged(QTreeWidgetItem* item, int column)
{
    if (m_guard > 0 || column != 0)
        return;

    const int kind = item->data(0, kKindRole).toInt();

    if (kind == KindHomeGroup || kind == KindDriveGroup) {
        setChildrenChecked(item, item->checkState(0));
        refreshAncestors(item->parent());
    } else {
        refreshAncestors(item->parent());
    }

    updateSummary();
    emit sourcesChanged();
}

void SourceChecklist::onItemExpanded(QTreeWidgetItem* item)
{
    if (!item || item->data(0, kKindRole).toInt() != KindDriveGroup)
        return;

    // Mutating the tree from inside the itemExpanded handler re-enters the
    // view while it is still updating itself. Defer to the event loop and
    // re-resolve the row by drive path so a refresh() in between cannot leave
    // us holding a dangling pointer.
    const QString root = item->data(0, kPathRole).toString();

    QTimer::singleShot(0, this, [this, root]() {
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* current = m_tree->topLevelItem(i);
            if (current->data(0, kPathRole).toString() == root &&
                current->data(0, kKindRole).toInt() == KindDriveGroup) {
                populateDriveChildren(current);
                requestSizes();
                return;
            }
        }
    });
}

void SourceChecklist::onSizeReady(const QString& path, qint64 bytes)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        QTreeWidgetItem* item = *it;
        if (item->data(0, kPathRole).toString() == path) {
            item->setData(0, kBytesRole, QVariant::fromValue<qint64>(bytes));
            item->setText(1, DriveManager::formatSize(bytes));
        }
        ++it;
    }

    updateSummary();
    emit sourcesChanged();
}

void SourceChecklist::onSelectAll()
{
    Guard guard(m_guard);

    // Set the CHILDREN of each top-level row, never the row's own box:
    // writing a parent's check state while the guard is held swallows the
    // itemChanged cascade, and the refreshAncestors() below would then
    // recompute the parent straight back from its untouched children.
    // Drive rows are not checkable at all, so setChildrenChecked() simply
    // skips them and ticks their folders directly.
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* top = m_tree->topLevelItem(i);
        setChildrenChecked(top, Qt::Checked);
        refreshAncestors(top);
    }

    updateSummary();
    emit sourcesChanged();
}

void SourceChecklist::onClearAll()
{
    Guard guard(m_guard);

    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* top = m_tree->topLevelItem(i);
        setChildrenChecked(top, Qt::Unchecked);
        refreshAncestors(top);
    }

    updateSummary();
    emit sourcesChanged();
}

QList<SourceEntry> SourceChecklist::checkedSources() const
{
    QList<SourceEntry> entries;

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        QTreeWidgetItem* item = *it;
        if (item->data(0, kKindRole).toInt() == KindFolder &&
            item->checkState(0) == Qt::Checked) {
            SourceEntry entry;
            entry.path = item->data(0, kPathRole).toString();
            entry.name = item->text(0);
            entry.bytes = item->data(0, kBytesRole).toLongLong();
            entries.append(entry);
        }
        ++it;
    }

    return entries;
}

int SourceChecklist::checkedCount() const
{
    return checkedSources().size();
}

qint64 SourceChecklist::checkedBytes() const
{
    qint64 total = 0;
    const QList<SourceEntry> entries = checkedSources();
    for (const SourceEntry& entry : entries) {
        if (entry.bytes > 0)
            total += entry.bytes;
    }
    return total;
}

bool SourceChecklist::anySizeUnknown() const
{
    const QList<SourceEntry> entries = checkedSources();
    for (const SourceEntry& entry : entries) {
        if (entry.bytes < 0)
            return true;
    }
    return false;
}

void SourceChecklist::updateSummary()
{
    const int count = checkedCount();
    const qint64 bytes = checkedBytes();
    const bool unknown = anySizeUnknown();

    if (count == 0) {
        m_summary->setText(QStringLiteral("Nothing selected"));
        return;
    }

    m_summary->setText(QString("%1 selected · %2%3")
                            .arg(count)
                            .arg(unknown ? QStringLiteral("~") : QString())
                            .arg(DriveManager::formatSize(bytes)));
}