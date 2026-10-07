#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QEventLoop>
#include <QTimer>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QTextStream>

#include "../src/core/TransferManager.h"

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

static bool writeFile(const QString& path, int sizeBytes)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    QByteArray block(1024 * 1024, 0);
    for (int i = 0; i < sizeBytes; i += block.size()) {
        if (f.write(block.constData(), qMin(block.size(), sizeBytes - i)) <= 0)
            return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    out << "=== TransferManager progress tests ===\n";
    out.flush();

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        out << "FATAL: could not create temp dir\n";
        return 2;
    }
    const QString root = QDir::cleanPath(tmp.path());
    const QString src = root + "/src";
    const QString dst = root + "/dst";

    const int bigSize = 40 * 1024 * 1024; // 40 MB: several 4 MB buffers
    writeFile(src + "/big.bin", bigSize);
    writeFile(src + "/empty.bin", 0);
    writeFile(src + "/dir/a.txt", 2048);
    writeFile(src + "/dir/b.txt", 4096);

    TransferManager mgr;
    mgr.setMaxConcurrent(1); // serial: isolates each transfer's update stream

    QSignalSpy allSpy(&mgr, &TransferManager::allCompleted);
    QSignalSpy updateSpy(&mgr, &TransferManager::itemUpdated);
    QSignalSpy progSpy(&mgr, &TransferManager::itemProgress);

    // --- big file: monotonic bytes, exact percent, bounded updates, ends 100% ---
    out << "-- big file --\n";
    TransferItem bigItem(src + "/big.bin", dst + "/big.bin", "big.bin",
                         bigSize, TransferMode::Copy, false);
    mgr.enqueue(bigItem);

    QElapsedTimer clock;
    clock.start();

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&mgr, &TransferManager::allCompleted, &loop, &QEventLoop::quit);
    timeout.start(30000);
    loop.exec();

    QList<QPair<qint64, qint64>> bigProg;            // (bytes, total) per update
    for (const QList<QVariant>& args : progSpy) {
        if (args.at(0).toUuid() == bigItem.id)
            bigProg.append({args.at(1).toLongLong(), args.at(2).toLongLong()});
    }
    const qint64 elapsedMs = clock.elapsed();

    // Engine reports once per 4 MB buffer; the worker throttles to ~10 Hz.
    // Allow a generous ceiling (one update per 100 ms of wall time + a few
    // for the forced final flush and rounding).
    const int maxUpdates = static_cast<int>(elapsedMs / 100) + 5;

    check(!bigProg.isEmpty(), QString("big file produces progress updates (got %1)")
                                  .arg(bigProg.size()));
    if (!bigProg.isEmpty()) {
        bool monotonic = true;
        bool exactPercent = true;
        bool boundedCount = bigProg.size() <= maxUpdates;
        for (int i = 0; i < bigProg.size(); ++i) {
            const qint64 bytes = bigProg.at(i).first;
            const qint64 total = bigProg.at(i).second;
            if (i > 0 && bytes < bigProg.at(i - 1).first)
                monotonic = false;
            if (total > 0 && bytes > total)
                exactPercent = false;
        }
        check(monotonic, "big file bytes are monotonic");
        check(exactPercent, "big file bytes never exceed reported total");
        check(boundedCount, QString("big file update count is throttled (%1 <= %2)")
                                .arg(bigProg.size()).arg(maxUpdates));
        check(bigProg.last().first == bigSize,
              QString("big file final update reports all bytes (%1/%2)")
                  .arg(bigProg.last().first).arg(bigSize));
    }
    check(mgr.item(bigItem.id).percent == 100,
          "big file ends at 100%");
    check(mgr.item(bigItem.id).status == TransferStatus::Completed,
          "big file completes");

    // itemUpdated percent must equal the byte ratio on every snapshot.
    bool percentConsistent = true;
    for (const QList<QVariant>& args : updateSpy) {
        const TransferItem it = args.at(0).value<TransferItem>();
        if (it.id == bigItem.id && it.totalBytes > 0) {
            const int expect = qBound(0, static_cast<int>((it.bytesTransferred * 100) / it.totalBytes), 100);
            if (it.percent != expect)
                percentConsistent = false;
        }
    }
    check(percentConsistent, "itemUpdated percent equals bytes/total on every snapshot");
    (void)elapsedMs;

    // --- empty file: still lands on 100% Completed ---
    out << "-- empty file --\n";
    updateSpy.clear();
    TransferItem emptyItem(src + "/empty.bin", dst + "/empty.bin", "empty.bin",
                           0, TransferMode::Copy, false);
    mgr.enqueue(emptyItem);
    timeout.start(30000);
    loop.exec();
    check(mgr.item(emptyItem.id).status == TransferStatus::Completed,
          "empty file completes");
    check(mgr.item(emptyItem.id).percent == 100,
          "empty file reaches 100%");

    // --- small directory with verification ---
    out << "-- small dir + verify --\n";
    updateSpy.clear();
    TransferItem dirItem(src + "/dir", dst + "/dir2", "dir",
                         2048 + 4096, TransferMode::Copy, true);
    mgr.enqueue(dirItem);
    timeout.start(30000);
    loop.exec();
    check(mgr.item(dirItem.id).status == TransferStatus::Completed,
          "small directory verifies and completes");
    check(QFileInfo::exists(dst + "/dir2/a.txt") && QFileInfo::exists(dst + "/dir2/b.txt"),
          "directory contents landed at the destination");

    // The GUI needs a visible "Verifying" phase: a verify-enabled transfer must
    // report TransferStatus::Verifying while the byte-compare runs (the worker
    // is otherwise silent between the copy's final 100% and the Completed
    // signal, which shows up in the app as the progress freezes at 100%).
    bool sawVerifying = false;
    bool verifyingAtFullPercent = false;
    bool sawVerifyStateVerifying = false;
    for (const QList<QVariant>& args : updateSpy) {
        const TransferItem it = args.at(0).value<TransferItem>();
        if (it.id == dirItem.id && it.status == TransferStatus::Verifying) {
            sawVerifying = true;
            if (it.percent == 100)
                verifyingAtFullPercent = true;
        }
        if (it.id == dirItem.id && it.verifyState == VerifyState::Verifying)
            sawVerifyStateVerifying = true;
    }
    check(sawVerifying, "verify-enabled transfer reports a Verifying status phase");
    check(verifyingAtFullPercent,
          "Verifying phase is reported after the copy reached 100%");
    check(sawVerifyStateVerifying,
          "verify column exposes a Verifying state while the byte-check runs");

    // The per-item byte-compare outcome lands on the queue row when done:
    // passed for a matching tree, and never set for transfers without verify.
    check(mgr.item(dirItem.id).verifyState == VerifyState::Passed,
          "byte-compare outcome is Passed for a matching tree");
    check(mgr.item(bigItem.id).verifyState == VerifyState::NotRequested,
          "transfer without verify keeps the verify column unset");

    // --- missing source fails cleanly ---
    out << "-- missing source --\n";
    TransferItem missingItem(root + "/nope.bin", dst + "/nope.bin", "nope.bin",
                             123, TransferMode::Copy, false);
    mgr.enqueue(missingItem);
    timeout.start(30000);
    loop.exec();
    check(mgr.item(missingItem.id).status == TransferStatus::Failed,
          "missing source reports Failed");

    out << "=== " << (g_checks - g_failures) << "/" << g_checks << " checks passed ===\n";
    if (g_failures > 0) {
        out << "RESULT: FAIL (" << g_failures << " failing)\n";
        return 1;
    }
    out << "RESULT: PASS\n";
    return 0;
}