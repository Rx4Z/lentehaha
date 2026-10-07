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
    // Home is always the first top-level row; every following top-level row
    // is a drive group (drives are now checkable whole-drive sources).
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        if (item->text(0) != QStringLiteral("Home"))
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

    // --- drive group: checkable whole-drive source ---
    QTreeWidgetItem* drive = findDriveGroup(tree);
    out << "  [diag] system drive excluded: " << DriveManager::systemDriveLetter()
        << ", non-system drives: " << DriveManager::enumerateDrives(false).size() << "\n";
    out.flush();

    if (!drive) {
        check(false, "a non-system drive group exists");
    } else {
        check(drive->text(0) != QStringLiteral("Home") && !drive->text(0).isEmpty(),
              "drive row is labelled");
        check(drive->flags().testFlag(Qt::ItemIsUserCheckable),
              "drive row IS checkable (whole-drive source)");

        // The row must carry real check-state data: Qt paints the checkbox
        // only when the CheckStateRole holds a value — the checkable flag
        // alone renders nothing (this is why the live app showed no box).
        check(drive->data(0, Qt::CheckStateRole).isValid(),
              "drive row has check-state data so its checkbox renders");

        // The title must name the drive like Windows does and include the
        // actual drive letter parsed from the root path — not the media
        // kind ("Fixed", "Removable", …).
        const QString driveLetter = drive->data(0, Qt::UserRole).toString().left(2);
        check(driveLetter.size() == 2 && drive->text(0).contains(driveLetter),
              QString("drive title '%1' contains the drive letter '%2'")
                  .arg(drive->text(0), driveLetter));

        QTreeWidgetItem* driveChild = drive->child(0);

        // The name attached to the transfer entry is the volume label (or
        // the bare letter for unlabeled volumes), never the media kind.
        const QString entryName = driveChild != nullptr
            ? driveChild->data(0, Qt::UserRole + 4).toString() : QString();
        check(!entryName.isEmpty() &&
                  entryName != QStringLiteral("Fixed") &&
                  entryName != QStringLiteral("Removable") &&
                  entryName != QStringLiteral("Network") &&
                  entryName != QStringLiteral("Optical"),
              QString("drive source name '%1' is the label/letter, not the media kind")
                  .arg(entryName));

        // The drive's child row is a REAL checkable whole-drive source (like
        // a Home folder), not a decorative stub: it must render the same
        // checkbox as every other source row.
        check(drive->childCount() == 1,
              QString("drive group has one source child (got %1)")
                  .arg(drive->childCount()));

        check(driveChild != nullptr &&
                  driveChild->flags().testFlag(Qt::ItemIsUserCheckable),
              "drive source child IS checkable");
        check(driveChild != nullptr && driveChild->data(0, Qt::CheckStateRole).isValid(),
              "drive source child has check-state data so its checkbox renders");
        check(driveChild != nullptr &&
                  driveChild->data(0, Qt::UserRole + 1).toInt() == 3,
              "drive source child is the KindDriveSource row");
        check(driveChild != nullptr &&
                  driveChild->text(0) == drive->data(0, Qt::UserRole).toString(),
              "drive source child shows the drive path");

        // Whole-drive sources default to OFF, exactly like a checked-off box.
        check(drive->checkState(0) == Qt::Unchecked &&
                  driveChild != nullptr && driveChild->checkState(0) == Qt::Unchecked,
              "drive group and its source child start unchecked");

        // Expanding must NOT list extra children.
        tree->expandItem(drive);
        QCoreApplication::processEvents();
        check(drive->childCount() == 1,
              QString("expanding the drive does not list extras (got %1 children)")
                  .arg(drive->childCount()));
        check(drive->child(0) == driveChild,
              "the drive source child survives expansion");

        // Checking the drive GROUP cascades to its whole-drive source child.
        const int before = checklist.checkedCount();
        drive->setCheckState(0, Qt::Checked);
        QCoreApplication::processEvents();
        check(checklist.checkedCount() == before + 1,
              QString("checking the drive group adds one source (%1 -> %2)")
                  .arg(before).arg(checklist.checkedCount()));
        check(driveChild != nullptr && driveChild->checkState(0) == Qt::Checked,
              "the drive source child becomes checked with the group");

        const QList<SourceEntry> sources = checklist.checkedSources();
        bool foundDrive = false;
        for (const SourceEntry& entry : sources) {
            if (entry.path == drive->data(0, Qt::UserRole).toString()) {
                foundDrive = true;
                check(!entry.name.isEmpty(), "drive source has a non-empty name");
                check(entry.bytes > 0, QString("drive source reports used bytes (got %1)")
                                           .arg(entry.bytes));
            }
        }
        check(foundDrive, "checkedSources includes the whole drive as one entry");

        // Toggling ONLY the child must re-derive the group's own box.
        driveChild->setCheckState(0, Qt::Unchecked);
        QCoreApplication::processEvents();
        check(drive->checkState(0) == Qt::Unchecked,
              "drive group mirrors a fully-unchecked source child");
        driveChild->setCheckState(0, Qt::Checked);
        QCoreApplication::processEvents();
        check(drive->checkState(0) == Qt::Checked,
              "drive group mirrors a fully-checked source child");
    }

    // --- Verdict: Select all / Clear must reach Home AND the drive rows ---
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

    // State at this point: Home is partially checked (5/6), the drive row is
    // checked. Clear must empty EVERYTHING, including the drive's own box.
    if (clearBtn) {
        QTest::mouseClick(clearBtn, Qt::LeftButton);
        QCoreApplication::processEvents();

        check(checklist.checkedCount() == 0,
              QString("Clear empties every checkbox (got %1)").arg(checklist.checkedCount()));
        check(home->checkState(0) == Qt::Unchecked, "Clear leaves Home unchecked");
        check(checkedUnder(home) == 0,
              QString("Clear empties Home's folders (got %1)").arg(checkedUnder(home)));
        if (drive) {
            check(drive->checkState(0) == Qt::Unchecked,
                  "Clear unchecks the drive row itself");
            if (drive->childCount() > 0)
                check(drive->child(0)->checkState(0) == Qt::Unchecked,
                      "Clear unchecks the drive source child too");
        }
        QLabel* summary = checklist.findChild<QLabel*>(QStringLiteral("statusLabel"));
        check(summary != nullptr && summary->text() == QStringLiteral("Nothing selected"),
              "summary reads Nothing selected after Clear");
    }

    // From a fully cleared state, Select all must tick Home's folders, Home's
    // own box, and every drive row.
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
            check(drive->checkState(0) == Qt::Checked,
                  "Select all ticks the drive row itself");
            check(drive->childCount() > 0 && drive->child(0)->checkState(0) == Qt::Checked,
                  "Select all ticks the drive source child too");
            const int driveCount = tree->topLevelItemCount() - 1;
            check(checklist.checkedCount() == home->childCount() + driveCount,
                  QString("summary counts Home folders + whole drives (got %1, want %2)")
                      .arg(checklist.checkedCount())
                      .arg(home->childCount() + driveCount));
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