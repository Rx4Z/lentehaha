// Rendered-geometry regression tests for the visual fixes:
//   1. transfer-table header labels start in the same column as their cells
//   2. the Copy/Move radios use one circular indicator in both states
//   3. source-tree rows share a single text column whether or not they carry a
//      checkbox, and selection does not recolour the checkbox indicator
//   4. the "Verify after transfer" checkbox shows a visible checked state
// Everything is measured on offscreen renders, so these checks fail on any
// style-sheet change that reintroduces the misalignment.
#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QPainter>
#include <QRadioButton>
#include <QTableWidget>
#include <QTextStream>
#include <QTreeWidget>

#include "../src/common/Theme.h"
#include "../src/core/TransferTask.h"
#include "../src/ui/SourceChecklist.h"
#include "../src/ui/TransferQueueWidget.h"

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

static QImage renderWidget(QWidget* w)
{
    QImage img(w->size(), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    w->render(&p);
    p.end();
    return img;
}

static bool closeTo(const QColor& c, const QColor& t, int tol)
{
    return qAbs(c.red() - t.red()) <= tol &&
           qAbs(c.green() - t.green()) <= tol &&
           qAbs(c.blue() - t.blue()) <= tol;
}

// Leftmost / rightmost pixel inside rect whose colour is close to target.
static int minXOfColor(const QImage& img, const QRect& r, const QColor& target, int tol = 70)
{
    const int x0 = qMax(0, r.left()), x1 = qMin(img.width() - 1, r.right());
    const int y0 = qMax(0, r.top()), y1 = qMin(img.height() - 1, r.bottom());
    for (int x = x0; x <= x1; ++x)
        for (int y = y0; y <= y1; ++y)
            if (closeTo(img.pixelColor(x, y), target, tol))
                return x;
    return -1;
}

static int maxXOfColor(const QImage& img, const QRect& r, const QColor& target, int tol = 70)
{
    const int x0 = qMax(0, r.left()), x1 = qMin(img.width() - 1, r.right());
    const int y0 = qMax(0, r.top()), y1 = qMin(img.height() - 1, r.bottom());
    for (int x = x1; x >= x0; --x)
        for (int y = y0; y <= y1; ++y)
            if (closeTo(img.pixelColor(x, y), target, tol))
                return x;
    return -1;
}

static int countOfColor(const QImage& img, const QRect& r, const QColor& target, int tol = 70)
{
    int n = 0;
    const int x0 = qMax(0, r.left()), x1 = qMin(img.width() - 1, r.right());
    const int y0 = qMax(0, r.top()), y1 = qMin(img.height() - 1, r.bottom());
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            if (closeTo(img.pixelColor(x, y), target, tol))
                ++n;
    return n;
}

static const QColor kText(192, 202, 245);      // #c0caf5
static const QColor kBlue(122, 162, 247);      // #7aa2f7
static const QColor kRowSelected(51, 70, 124); // #33467c
static const QColor kWindow(26, 27, 38);       // #1a1b26

// ---------------------------------------------------------------- table ----
static void testTableHeaders()
{
    QTextStream(stdout) << "-- transfer table: header label vs cell text --\n";
    TransferQueueWidget queue;
    queue.setStyleSheet(Theme::instance().stylesheet());
    queue.resize(860, 260);
    queue.show();

    TransferItem item;
    item.displayName = QStringLiteral("Sample file.bin");
    item.totalBytes = 123456789;
    item.percent = 42;
    item.speedBps = 5 * 1024 * 1024;
    item.status = TransferStatus::InProgress;
    queue.addItem(item);
    QCoreApplication::processEvents();

    QTableWidget* table = queue.findChild<QTableWidget*>();
    check(table != nullptr, "transfer table is present");
    if (!table)
        return;

    QHeaderView* header = table->horizontalHeader();
    check(header->defaultAlignment().testFlag(Qt::AlignLeft),
          "table header labels are left aligned");
    check(!header->defaultAlignment().testFlag(Qt::AlignHCenter),
          "table header labels are not centred");

    const QImage img = renderWidget(&queue);
    const QStringList labels = { "Name", "Status", "Progress", "Size", "Speed", "ETA" };

    for (int c = 0; c < table->columnCount(); ++c) {
        const QRect sec(header->mapTo(&queue,
                                      QPoint(header->sectionViewportPosition(c), 0)),
                        QSize(header->sectionSize(c), header->height()));
        const QRect hband(sec.left(), sec.top() + 2, sec.width(), sec.height() - 5);
        const int hText = minXOfColor(img, hband, kText);

        const QRect cr = table->visualRect(table->model()->index(0, c));
        const QRect cell(table->viewport()->mapTo(&queue, cr.topLeft()), cr.size());
        const QRect cband(cell.left(), cell.top() + 2, cell.width(), cell.height() - 5);
        const int cText = minXOfColor(img, cband, kText);

        check(hText >= 0 && cText >= 0, labels.value(c) + ": header and cell text rendered");
        if (hText < 0 || cText < 0)
            continue;
        const int hInset = hText - sec.left();
        const int cInset = cText - cell.left();
        check(hInset == cInset,
              labels.value(c) + ": header starts on the cell's column (" +
                  QString::number(hInset) + "px vs " + QString::number(cInset) + "px)");
    }
}

// ----------------------------------------------------------- mode radios ----
static void testModeRadios()
{
    QTextStream(stdout) << "-- mode selector: Copy vs Move indicator --\n";

    QWidget host;
    host.setStyleSheet(Theme::instance().stylesheet());
    auto* root = new QVBoxLayout(&host);
    root->setContentsMargins(16, 14, 16, 12);

    auto* group = new QGroupBox(QStringLiteral("Mode"), &host);
    auto* modeLayout = new QHBoxLayout(group);
    modeLayout->setContentsMargins(10, 4, 10, 6);
    auto* copy = new QRadioButton(QStringLiteral("Copy"), &host);
    auto* move = new QRadioButton(QStringLiteral("Move"), &host);
    copy->setChecked(true);
    modeLayout->addWidget(copy);
    modeLayout->addWidget(move);
    root->addWidget(group);
    root->addStretch(1);

    host.resize(320, 140);
    host.show();
    QCoreApplication::processEvents();

    const QImage img = renderWidget(&host);

    struct Measure { QRadioButton* button; QString name; QRect box; int blue; };
    QList<Measure> measures;
    for (auto* btn : { copy, move }) {
        const QRect gr(btn->mapTo(&host, QPoint(0, 0)), btn->size());
        const QRect ind(gr.left(), gr.top(), 20, gr.height());

        int minX = -1, maxX = -1, minY = -1, maxY = -1;
        for (int y = ind.top(); y <= ind.bottom(); ++y)
            for (int x = ind.left(); x <= ind.right(); ++x)
                if (!closeTo(img.pixelColor(x, y), kWindow, 24)) {
                    if (minX < 0 || x < minX)
                        minX = x;
                    if (maxX < x)
                        maxX = x;
                    if (minY < 0 || y < minY)
                        minY = y;
                    if (maxY < y)
                        maxY = y;
                }
        measures.append({ btn, btn == copy ? QStringLiteral("Copy") : QStringLiteral("Move"),
                          minX >= 0 ? QRect(QPoint(minX, minY), QPoint(maxX, maxY)) : QRect(),
                          countOfColor(img, ind, kBlue, 60) });
    }

    const Measure& checked = measures.at(0);
    const Measure& unchecked = measures.at(1);
    check(checked.button->isChecked() && !unchecked.button->isChecked(),
          "Copy starts checked, Move starts unchecked");
    check(checked.box.isValid() && unchecked.box.isValid(),
          "both radios render an indicator");
    check(checked.box.width() == unchecked.box.width() &&
              checked.box.height() == unchecked.box.height(),
          QString("checked and unchecked indicators are the same size (%1x%2 vs %3x%4)")
              .arg(checked.box.width()).arg(checked.box.height())
              .arg(unchecked.box.width()).arg(unchecked.box.height()));
    check(checked.box.width() == checked.box.height(),
          "indicator box is square, so the rounded corners read as a circle");
    check(checked.blue > 100, "checked indicator is filled (" + QString::number(checked.blue) +
                                  " primary pixels)");
    check(unchecked.blue == 0, "unchecked indicator has no primary fill");
}

// ------------------------------------------------------------ tree rows -----
static QTreeWidgetItem* findNonCheckableTopLevel(QTreeWidget* tree)
{
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = tree->topLevelItem(i);
        if (!item->flags().testFlag(Qt::ItemIsUserCheckable))
            return item;
    }
    return nullptr;
}

static int textXOf(const QImage& img, QTreeWidget* tree, QTreeWidgetItem* item,
                   const QColor& colour)
{
    const QRect vr = tree->visualItemRect(item);
    const QRect r(tree->viewport()->mapTo(tree, vr.topLeft()), vr.size());
    return minXOfColor(img, r, colour);
}

static void testSourceTree()
{
    QTextStream(stdout) << "-- source tree: text column and indicator --\n";

    SourceChecklist checklist(nullptr);
    checklist.setStyleSheet(Theme::instance().stylesheet());
    checklist.resize(680, 540);
    checklist.show();
    QCoreApplication::processEvents();

    QTreeWidget* tree = checklist.tree();
    check(tree != nullptr, "source tree is present");
    if (!tree)
        return;

    QTreeWidgetItem* home = tree->topLevelItem(0);
    QTreeWidgetItem* drive = findNonCheckableTopLevel(tree);
    check(home != nullptr, "Home group exists");
    check(drive != nullptr, "a non-checkable drive group exists");
    if (!home || !drive)
        return;

    tree->expandItem(drive);
    QCoreApplication::processEvents();   // deferred drive population
    QCoreApplication::processEvents();

    // Distinct colours so text cannot be confused with checkbox chrome.
    home->setForeground(0, QColor(255, 0, 0));
    if (home->childCount() > 0)
        home->child(0)->setForeground(0, QColor(0, 0, 255));
    drive->setForeground(0, QColor(0, 255, 0));
    if (drive->childCount() > 0)
        drive->child(0)->setForeground(0, QColor(255, 255, 0));
    QCoreApplication::processEvents();

    const QImage img = renderWidget(tree);

    const int homeText = textXOf(img, tree, home, QColor(255, 0, 0));
    const int driveText = textXOf(img, tree, drive, QColor(0, 255, 0));
    check(homeText > 0 && driveText > 0, "level-0 rows rendered their text");
    check(homeText == driveText,
          "checkbox and checkbox-less rows share one text column (" +
              QString::number(homeText) + "px vs " + QString::number(driveText) + "px)");

    if (home->childCount() > 0 && drive->childCount() > 0) {
        const int homeChild = textXOf(img, tree, home->child(0), QColor(0, 0, 255));
        const int driveChild = textXOf(img, tree, drive->child(0), QColor(255, 255, 0));
        check(homeChild == driveChild,
              "level-1 rows share one text column (" + QString::number(homeChild) +
                  "px vs " + QString::number(driveChild) + "px)");
    }

    check(tree->visualItemRect(home).height() == tree->visualItemRect(drive).height(),
          "rows have one uniform height");

    QHeaderView* th = tree->header();
    check(th->defaultAlignment().testFlag(Qt::AlignLeft), "tree header labels are left aligned");

    // Selection must not recolour an unchecked indicator: its interior has to
    // track the row background so the box looks identical in both states.
    const Qt::CheckState before = home->checkState(0);
    home->setCheckState(0, Qt::Unchecked);
    tree->selectionModel()->clear();
    QCoreApplication::processEvents();
    const QImage normal = renderWidget(tree);

    tree->scrollToItem(home);
    tree->setCurrentItem(home, 0,
                         QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    QCoreApplication::processEvents();
    const QImage selected = renderWidget(tree);

    const QRect vr = tree->visualItemRect(home);
    const QRect r(tree->viewport()->mapTo(tree, vr.topLeft()), vr.size());
    const QPoint boxInterior = r.topLeft() + QPoint(15, 11);   // inside the 17px indicator
    const QPoint plainRow = r.topLeft() + QPoint(45, 11);      // row background

    check(normal.pixelColor(boxInterior) == normal.pixelColor(plainRow),
          "unchecked indicator blends into the normal row (" +
              normal.pixelColor(boxInterior).name() + ")");
    check(selected.pixelColor(boxInterior) == selected.pixelColor(plainRow),
          "unchecked indicator blends into the selected row (" +
              selected.pixelColor(boxInterior).name() + ")");

    const QRect band(0, r.top(), tree->viewport()->width(), r.height());
    const int hiLeft = minXOfColor(selected, band, kRowSelected, 12);
    const int hiRight = maxXOfColor(selected, band, kRowSelected, 12);
    check(hiLeft >= band.left() && hiLeft <= band.left() + 1 && hiRight >= band.right() - 1,
          "selected row highlight still spans the whole row");

    home->setCheckState(0, before);
    tree->selectionModel()->clear();
    QCoreApplication::processEvents();
}

// ------------------------------------------------------- verify checkbox ----
static void testVerifyCheckbox()
{
    QTextStream(stdout) << "-- verify checkbox: checked vs unchecked --\n";

    QWidget host;
    host.setStyleSheet(Theme::instance().stylesheet());
    auto* lay = new QVBoxLayout(&host);
    auto* on = new QCheckBox(QStringLiteral("Verify after transfer (byte compare)"), &host);
    auto* off = new QCheckBox(QStringLiteral("Verify after transfer (byte compare)"), &host);
    on->setChecked(true);
    off->setChecked(false);
    lay->addWidget(on);
    lay->addWidget(off);
    host.resize(420, 90);
    host.show();
    QCoreApplication::processEvents();

    const QImage img = renderWidget(&host);
    int blueOn = -1, blueOff = -1;
    for (auto* box : { on, off }) {
        const QRect r(box->mapTo(&host, QPoint(0, 0)), box->size());
        const int blue = countOfColor(img, QRect(r.left(), r.top(), 20, r.height()), kBlue, 60);
        if (box->isChecked())
            blueOn = blue;
        else
            blueOff = blue;
    }
    check(blueOn > 0, "checked verify checkbox shows its state (" +
                          QString::number(blueOn) + " primary pixels)");
    check(blueOff == 0, "unchecked verify checkbox shows no fill");
}

int main(int argc, char** argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QTextStream out(stdout);

    out << "=== Visual alignment tests ===\n";
    out.flush();

    testTableHeaders();
    testModeRadios();
    testSourceTree();
    testVerifyCheckbox();

    out << "\n" << g_checks << " checks, " << g_failures << " failures\n";
    out.flush();
    return g_failures == 0 ? 0 : 1;
}
