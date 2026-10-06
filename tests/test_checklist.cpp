#include <QApplication>
#include <QTreeWidget>
#include <QTest>
#include <QTextStream>

#include "../src/ui/SourceChecklist.h"
#include "../src/core/DriveManager.h"

static int g_failures = 0;
static int g_checks = 0;

static void check(bool condition, const QString& what)
{
    ++g_checks;
    QTextStream(stdout) << (condition ? "  ok:   " : "  FAIL: ") << what << "\n";
    QTextStream(stdout).flush();
    if (!condition)
        ++g_failures;
}

static QTreeWidgetItem* findDriveGroup(QTreeWidget* tree)
{
    // Drive rows are the only top-level entries that are NOT checkable
    // (they are expand-only containers, not sources themselves).
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        if (!item->flags().testFlag(Qt::ItemIsUserCheckable))
            return item;
    }
    return nullptr;
}

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTextStream out(stdout);

    out << "=== SourceChecklist tests ===\n";
    out.flush();

    // Pass a null scanner: sizing is irrelevant here and this keeps the test
    // free of background threads.
    SourceChecklist checklist(nullptr);

    QTreeWidget* tree = checklist.tree();
    check(tree != nullptr, "tree is exposed");

    // --- Home group ---
    QTreeWidgetItem* home = tree->topLevelItem(0);
    check(home != nullptr && home->text(0) == QStringLiteral("Home"), "Home group is first");
    check(home != nullptr && home->childCount() == 6, "Home has 6 known folders");
    check(home != nullptr && home->checkState(0) == Qt::Checked, "Home is checked by default");

    int checked = 0;
    for (int i = 0; i < home->childCount(); ++i)
        if (home->child(i)->checkState(0) == Qt::Checked)
            ++checked;
    check(checked == 6, QString("all 6 Home folders checked (got %1)").arg(checked));
    check(checklist.checkedCount() == 6,
          QString("checkedSources reports 6 (got %1)").arg(checklist.checkedCount()));

    // Unchecking one Home child must partially check the parent.
    home->child(0)->setCheckState(0, Qt::Unchecked);
    check(home->checkState(0) == Qt::PartiallyChecked,
          "Home becomes PartiallyChecked when one child is cleared");
    check(checklist.checkedCount() == 5,
          QString("checkedSources drops to 5 (got %1)").arg(checklist.checkedCount()));

    // --- drive group is expandable ---
    QTreeWidgetItem* drive = findDriveGroup(tree);
    out << "  [diag] system drive excluded: " << DriveManager::systemDriveLetter()
        << ", non-system drives: " << DriveManager::enumerateDrives(false).size() << "\n";
    out.flush();

    if (!drive) {
        check(false, "a non-system drive group exists to expand");
    } else {
        check(drive->text(0) != QStringLiteral("Home") && !drive->text(0).isEmpty(),
              "drive row is labelled");
        check(!drive->flags().testFlag(Qt::ItemIsUserCheckable),
              "drive row itself is NOT checkable (expand-only)");

        // The expander only renders if the item has a child.
        check(drive->childCount() == 1,
              QString("drive row has a placeholder child so an expander is drawn (got %1)")
                  .arg(drive->childCount()));

        const QString root = drive->data(0, Qt::UserRole).toString();
        const int expected = DriveManager::topLevelFolders(root).size();

        // --- the real user path: click the expander arrow ---
        // NB: visualItemRect() starts at the item TEXT. The branch (expander)
        // is drawn to the LEFT of it, one indentation to the left, so the
        // click has to target rowRect.left() - 10 rather than rowRect.left().
        tree->resize(640, 520);
        tree->show();
        QCoreApplication::processEvents();
        tree->scrollToItem(drive);
        QCoreApplication::processEvents();

        const QRect rowRect = tree->visualItemRect(drive);
        out << "  [diag] visualItemRect=" << rowRect.x() << "," << rowRect.y()
            << " " << rowRect.width() << "x" << rowRect.height()
            << " -> clicking branch at x=" << (rowRect.left() - 10) << "\n";
        out.flush();

        QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier,
                          QPoint(rowRect.left() - 10, rowRect.center().y()));
        QCoreApplication::processEvents();

        out << "  [diag] after branch click: expanded=" << drive->isExpanded()
            << " children=" << drive->childCount() << "\n";
        out.flush();

        check(drive->isExpanded(),
              "clicking the expander indicator expands the drive row");
        check(drive->childCount() == expected,
              QString("branch click lazy-loads every top-level folder (%1 vs %2)")
                  .arg(drive->childCount()).arg(expected));

        // Safety net: if a platform ever stops honouring the synthetic click,
        // expand programmatically so the remaining assertions still run.
        if (drive->childCount() <= 1) {
            tree->expandAll();
            QCoreApplication::processEvents();
            out << "  [diag] fell back to expandAll\n";
        }

        check(drive->childCount() == expected,
              QString("expanding loads every top-level folder (%1 vs %2)")
                  .arg(drive->childCount()).arg(expected));

        // Placeholder must be gone, real folders present.
        bool hasPlaceholder = false;
        bool allCheckable = drive->childCount() > 0;
        for (int i = 0; i < drive->childCount(); ++i) {
            QTreeWidgetItem* child = drive->child(i);
            if (child->text(0) == QStringLiteral("Loading…"))
                hasPlaceholder = true;
            if (!child->flags().testFlag(Qt::ItemIsUserCheckable))
                allCheckable = false;
        }
        check(!hasPlaceholder, "placeholder is replaced by real folders");
        check(allCheckable, "every drive folder is checkable");

        // System folders must not be offered.
        bool offersSystemFolder = false;
        for (int i = 0; i < drive->childCount(); ++i) {
            const QString name = drive->child(i)->text(0);
            if (name.startsWith(QLatin1Char('$')) ||
                name.compare(QStringLiteral("System Volume Information"), Qt::CaseInsensitive) == 0)
                offersSystemFolder = true;
        }
        check(!offersSystemFolder, "uncopyable system folders are filtered out");

        // THIS is the reported bug: checking a folder under a drive.
        const int before = checklist.checkedCount();
        drive->child(0)->setCheckState(0, Qt::Checked);
        check(checklist.checkedCount() == before + 1,
              QString("checking a drive folder works (%1 -> %2)")
                  .arg(before).arg(checklist.checkedCount()));

        drive->child(1)->setCheckState(0, Qt::Checked);
        check(checklist.checkedCount() == before + 2, "a second drive folder also checks");
    }

    // --- Verdict: Select all / Clear must reach Home AND drive children ---
    QPushButton* selectAllBtn = nullptr;
    QPushButton* clearBtn = nullptr;
    for (QPushButton* b : checklist.findChildren<QPushButton*>()) {
        if (b->text() == QStringLiteral("Select all"))
            selectAllBtn = b;
        else if (b->text() == QStringLiteral("Clear"))
            clearBtn = b;
    }
    check(selectAllBtn != nullptr && clearBtn != nullptr,
          "Select all and Clear buttons are findable");

    const auto checkedUnder = [](QTreeWidgetItem* parent) {
        int n = 0;
        for (int i = 0; i < parent->childCount(); ++i)
            if (parent->child(i)->checkState(0) == Qt::Checked)
                ++n;
        return n;
    };

    // State at this point: Home is partially checked (5/6), two drive
    // folders are checked. Clear must empty EVERYTHING, not just drives.
    if (clearBtn) {
        QTest::mouseClick(clearBtn, Qt::LeftButton);
        QCoreApplication::processEvents();

        check(checklist.checkedCount() == 0,
              QString("Clear empties every checkbox (got %1)").arg(checklist.checkedCount()));
        check(home->checkState(0) == Qt::Unchecked, "Clear leaves Home unchecked");
        check(checkedUnder(home) == 0,
              QString("Clear empties Home's folders (got %1)").arg(checkedUnder(home)));
        if (drive)
            check(checkedUnder(drive) == 0,
                  QString("Clear empties drive folders (got %1)").arg(checkedUnder(drive)));
        QLabel* summary = checklist.findChild<QLabel*>(QStringLiteral("statusLabel"));
        check(summary != nullptr && summary->text() == QStringLiteral("Nothing selected"),
              "summary reads Nothing selected after Clear");
    }

    // From a fully cleared state, Select all must tick Home's folders
    // (Home itself shows Checked) and every drive folder.
    if (selectAllBtn) {
        QTest::mouseClick(selectAllBtn, Qt::LeftButton);
        QCoreApplication::processEvents();

        check(checkedUnder(home) == home->childCount(),
              QString("Select all ticks all Home folders (got %1/%2)")
                  .arg(checkedUnder(home)).arg(home->childCount()));
        check(home->checkState(0) == Qt::Checked,
              QString("Select all shows Home checked (got state %1)")
                  .arg(int(home->checkState(0))));
        if (drive) {
            check(checkedUnder(drive) == drive->childCount(),
                  QString("Select all ticks every drive folder (got %1/%2)")
                      .arg(checkedUnder(drive)).arg(drive->childCount()));
            check(checklist.checkedCount() == home->childCount() + drive->childCount(),
                  QString("summary counts Home + drive folders (got %1, want %2)")
                      .arg(checklist.checkedCount())
                      .arg(home->childCount() + drive->childCount()));
        }
    }

    out << "=== " << (g_checks - g_failures) << "/" << g_checks << " checks passed ===\n";
    if (g_failures > 0) {
        out << "RESULT: FAIL (" << g_failures << " failing)\n";
        return 1;
    }
    out << "RESULT: PASS\n";
    return 0;
}
