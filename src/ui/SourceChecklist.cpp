#include "SourceChecklist.h"
#include "../core/SizeScanner.h"
#include "../common/Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QtAlgorithms>

namespace {
constexpr int kPathRole = Qt::UserRole;
constexpr int kKindRole = Qt::UserRole + 1;
constexpr int kBytesRole = Qt::UserRole + 2;
constexpr int kSizeTextRole = Qt::UserRole + 3;
constexpr int kNameRole = Qt::UserRole + 4;

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

    // Name the row the way Windows does: volume label plus letter, e.g.
    // "Cr0w (E:)". Unlabeled volumes show just the letter ("E:").
    const QString title = drive.label.isEmpty()
        ? drive.letter
        : QStringLiteral("%1 (%2)").arg(drive.label, drive.letter);
    const QString driveName = drive.label.isEmpty()
        ? QString(drive.letter).remove(QLatin1Char(':'))
        : drive.label;
    item->setText(0, title);
    item->setText(1, DriveManager::formatSize(drive.freeBytes) + QStringLiteral(" free"));
    item->setData(0, kPathRole, drive.path);
    item->setData(0, kKindRole, KindDriveGroup);
    item->setData(0, kBytesRole, QVariant::fromValue<qint64>(drive.totalBytes - drive.freeBytes));
    item->setForeground(0, Theme::instance().text());
    item->setForeground(1, Theme::instance().textMuted());
    QFont f = item->font(0);
    f.setBold(true);
    item->setFont(0, f);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    // ItemIsUserCheckable alone paints nothing: Qt only draws the checkbox
    // once the CheckStateRole holds an actual value. The group's own box is
    // a tri-state mirror of its source child, so start both unchecked.
    item->setCheckState(0, Qt::Unchecked);

    // The whole-drive source is a REAL checkable child row, exactly like a
    // Home folder — so it renders the standard checkbox (not a decorative
    // non-checkable stub) and carries the entry name/size used by the
    // transfer. Checking either box keeps the other in sync.
    auto* source = new QTreeWidgetItem(item);
    const QString usedText = DriveManager::formatSize(drive.totalBytes - drive.freeBytes);
    source->setText(0, drive.path);
    source->setText(1, usedText);
    source->setData(0, kPathRole, drive.path);
    source->setData(0, kKindRole, KindDriveSource);
    source->setData(0, kBytesRole, QVariant::fromValue<qint64>(drive.totalBytes - drive.freeBytes));
    source->setData(0, kNameRole, driveName);
    source->setForeground(0, Theme::instance().textMuted());
    source->setForeground(1, Theme::instance().textMuted());
    source->setFlags(source->flags() | Qt::ItemIsUserCheckable);
    // Without an actual CheckStateRole value Qt draws no checkbox even on a
    // checkable item, so give the source child a real (unchecked) state.
    source->setCheckState(0, Qt::Unchecked);

    return item;
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

            // Only re-derive a parent's state when it actually HAS checkable
            // children. A leaf-checkable row (a whole-drive source or a Home
            // group with no folders) must keep whatever the user chose.
            if (total > 0) {
                Qt::CheckState state = Qt::Unchecked;
                if (checked == total)
                    state = Qt::Checked;
                else if (checked > 0)
                    state = Qt::PartiallyChecked;
                item->setCheckState(0, state);
            }
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

    // Set each top-level row's OWN box when it is checkable (Home and now the
    // drive rows, which are whole-drive sources) and then its children, then
    // recompute ancestors. Setting the parent's state under the guard swallows
    // the itemChanged cascade; refreshAncestors() below re-derives it from the
    // children, so the two agree.
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* top = m_tree->topLevelItem(i);
        if (top->flags().testFlag(Qt::ItemIsUserCheckable))
            top->setCheckState(0, Qt::Checked);
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
        if (top->flags().testFlag(Qt::ItemIsUserCheckable))
            top->setCheckState(0, Qt::Unchecked);
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
        if (item->checkState(0) != Qt::Checked)
            { ++it; continue; }

        const int kind = item->data(0, kKindRole).toInt();
        if (kind == KindFolder || kind == KindDriveSource) {
            SourceEntry entry;
            entry.path = item->data(0, kPathRole).toString();
            entry.bytes = item->data(0, kBytesRole).toLongLong();
            if (kind == KindDriveSource)
                entry.name = item->data(0, kNameRole).toString();
            else
                entry.name = item->text(0);
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