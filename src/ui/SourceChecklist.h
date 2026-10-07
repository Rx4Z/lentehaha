#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include "../core/DriveManager.h"

class SizeScanner;

struct SourceEntry
{
    QString path;
    QString name;
    qint64 bytes = -1;
};

class SourceChecklist : public QWidget
{
    Q_OBJECT

public:
    explicit SourceChecklist(SizeScanner* scanner, QWidget* parent = nullptr);

    QList<SourceEntry> checkedSources() const;
    int checkedCount() const;
    qint64 checkedBytes() const;
    bool anySizeUnknown() const;

    void refresh();

    // Exposed so the widget's expand/lazy-load behaviour can be unit tested
    // without driving a real window.
    QTreeWidget* tree() const { return m_tree; }

signals:
    void sourcesChanged();

private slots:
    void onItemChanged(QTreeWidgetItem* item, int column);
    void onSizeReady(const QString& path, qint64 bytes);
    void onSelectAll();
    void onClearAll();

private:
    enum Kind { KindFolder, KindHomeGroup, KindDriveGroup, KindDriveSource };

    QTreeWidgetItem* buildHomeGroup();
    QTreeWidgetItem* buildFolderItem(const QString& name, const QString& path);
    QTreeWidgetItem* buildDriveGroup(const DriveInfo& drive);
    void populateAllDrives();
    void setChildrenChecked(QTreeWidgetItem* parent, Qt::CheckState state);
    void refreshAncestors(QTreeWidgetItem* item);
    void updateSummary();
    void requestSizes();

    QTreeWidget* m_tree;
    QLabel* m_summary;
    QPushButton* m_selectAllBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_refreshBtn;
    SizeScanner* m_scanner;
    int m_guard = 0;
};