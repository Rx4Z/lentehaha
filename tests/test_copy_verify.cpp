#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "../src/core/FileCopyEngine.h"
#include "../src/core/DriveManager.h"

#include <windows.h>

static int g_failures = 0;
static int g_checks = 0;

static void check(bool condition, const QString& what)
{
    ++g_checks;
    if (!condition) {
        ++g_failures;
        QTextStream(stdout) << "  FAIL: " << what << "\n";
    } else {
        QTextStream(stdout) << "  ok:   " << what << "\n";
    }
}

static bool writeFile(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    return f.write(data) == data.size();
}

static QByteArray makeData(int size, char seed)
{
    QByteArray data(size, seed);
    for (int i = 0; i < size; ++i)
        data[i] = static_cast<char>((seed + i) % 251);
    return data;
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    // Utility mode: show detected drives and their top-level folder counts.
    if (argc >= 2 && qstrcmp(argv[1], "--drives") == 0) {
        const QList<DriveInfo> drives = DriveManager::enumerateDrives();
        out << "system drive (excluded): " << DriveManager::systemDriveLetter() << "\n";
        out << "drives listed: " << drives.size() << "\n";
        for (const DriveInfo& d : drives) {
            const QStringList folders = DriveManager::topLevelFolders(d.path);
            out << "  " << d.letter << "  path=" << d.path
                << "  label=" << d.label
                << "  topLevelFolders=" << folders.size() << "\n";
            for (int i = 0; i < folders.size() && i < 5; ++i)
                out << "      - " << folders.at(i) << "\n";
        }
        return 0;
    }

    // Utility mode: copy a real directory and verify it, e.g.
    //   FluxTransferTests.exe --copy "C:/Users/me/Desktop" "E:/dest"
    if (argc >= 4 && qstrcmp(argv[1], "--copy") == 0) {
        const QString src = QString::fromLocal8Bit(argv[2]);
        const QString dst = QString::fromLocal8Bit(argv[3]);

        FileCopyEngine engine;
        qint64 total = 0;
        const CopyResult r = engine.copyDirectory(src, dst, [&](const CopyProgress& p) {
            total = p.totalBytes;
        });
        out << "copy result: " << static_cast<int>(r) << " (" << total << " bytes)\n";

        const qint64 engineSize = engine.calculateDirectorySize(src);
        out << "engine size: " << engineSize << "\n";

        const bool verified = engine.verifyDirectory(src, dst);
        out << "verified: " << (verified ? "YES" : "NO") << "\n";
        return (r == CopyResult::Success && verified) ? 0 : 1;
    }

    out << "=== FileCopyEngine copy/verify tests ===\n";

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        out << "FATAL: could not create temp dir\n";
        return 2;
    }

    const QString root = tmp.path();
    const QString src = root + "/src";
    const QString dst = root + "/dst";

    const QByteArray a = makeData(3 * 1024 * 1024 + 17, 7);   // multi-chunk, odd tail
    const QByteArray b = makeData(1024, 11);                  // exactly one buffer
    const QByteArray c;                                       // empty file

    writeFile(src + "/big.bin", a);
    writeFile(src + "/small.bin", b);
    writeFile(src + "/empty.bin", c);
    writeFile(src + "/nested/deep/leaf.txt", makeData(4096, 23));

    // --- directory copy ---
    FileCopyEngine engine;
    CopyResult result = CopyResult::Success;
    qint64 lastPercent = -1;
    qint64 lastTransferred = -1;
    bool percentOvershoot = false;
    bool monotonic = true;

    result = engine.copyDirectory(src, dst,
        [&](const CopyProgress& p) {
            if (p.totalBytes > 0 && p.percent > 100)
                percentOvershoot = true;
            if (p.bytesTransferred < lastTransferred)
                monotonic = false;
            lastPercent = p.percent;
            lastTransferred = p.bytesTransferred;
        });

    out << "-- copyDirectory --\n";
    check(result == CopyResult::Success, "copyDirectory returns Success");
    check(QFileInfo::exists(dst + "/big.bin"), "root file copied");
    check(QFileInfo::exists(dst + "/nested/deep/leaf.txt"), "nested structure preserved");
    check(!percentOvershoot, "reported percent never exceeds 100 (no double counting)");
    check(monotonic, "reported bytes are monotonic");
    check(lastPercent == 100, "final percent reaches 100");

    // --- size accounting ---
    out << "-- size accounting --\n";
    qint64 reported = 0;
    engine.copyDirectory(src, root + "/dst2", [&](const CopyProgress& p) {
        reported = p.totalBytes;
    });
    qint64 onDisk = 0;
    for (const QString& f : { QString("big.bin"), QString("small.bin"),
                              QString("empty.bin"), QString("nested/deep/leaf.txt") }) {
        onDisk += QFileInfo(src + "/" + f).size();
    }
    check(reported == onDisk,
          QString("totalBytes matches source bytes (%1 vs %2)").arg(reported).arg(onDisk));

    // --- verifyFile happy paths ---
    out << "-- verifyFile --\n";
    check(engine.verifyFile(src + "/big.bin", dst + "/big.bin"), "multi-chunk file verifies");
    check(engine.verifyFile(src + "/small.bin", dst + "/small.bin"), "single-buffer file verifies");
    check(engine.verifyFile(src + "/empty.bin", dst + "/empty.bin"), "empty file verifies");
    check(!engine.verifyFile(src + "/missing.bin", dst + "/big.bin"), "missing source fails verification");

    // --- verifyFile must detect corruption ---
    // Same length, one flipped byte -> only a real byte compare can catch this.
    QByteArray corrupt = a;
    corrupt[12345] = static_cast<char>(corrupt[12345] ^ 0xFF);
    writeFile(dst + "/big.bin", corrupt);
    check(!engine.verifyFile(src + "/big.bin", dst + "/big.bin"),
          "same-size single-byte corruption is detected");

    // Size truncation must be detected.
    writeFile(dst + "/big.bin", a.left(a.size() - 1024));
    check(!engine.verifyFile(src + "/big.bin", dst + "/big.bin"), "size mismatch is detected");

    // Missing destination must be detected.
    QFile::remove(dst + "/small.bin");
    check(!engine.verifyFile(src + "/small.bin", dst + "/small.bin"), "missing destination fails verification");

    // --- verifyDirectory ---
    out << "-- verifyDirectory --\n";
    FileCopyEngine fresh;
    fresh.copyDirectory(src, root + "/dst3");
    check(fresh.verifyDirectory(src, root + "/dst3"), "identical trees verify");

    QFile::remove(root + "/dst3/nested/deep/leaf.txt");
    check(!fresh.verifyDirectory(src, root + "/dst3"), "missing nested file is detected");

    fresh.copyDirectory(src, root + "/dst4");
    QFile::remove(root + "/dst4/small.bin");
    writeFile(root + "/dst4/extra.txt", makeData(10, 3));
    check(!fresh.verifyDirectory(src, root + "/dst4"), "extra/missing file count mismatch is detected");

    // --- regression: hidden/system files (e.g. desktop.ini on Desktop) ---
    // Traversals used to disagree about QDir::Hidden, so a hidden file was
    // skipped by the copy but still expected by verifyDirectory, which made
    // every transfer of a real Desktop folder fail verification.
    out << "-- hidden/system file regression --\n";
    const QString hiddenSrc = root + "/hidden-src";
    writeFile(hiddenSrc + "/visible.txt", makeData(512, 5));
    writeFile(hiddenSrc + "/desktop.ini", makeData(282, 9));
    writeFile(hiddenSrc + "/nested/.secret", makeData(64, 13));

    SetFileAttributesW(reinterpret_cast<const wchar_t*>(hiddenSrc.toStdWString().c_str()),
                       FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    SetFileAttributesW(reinterpret_cast<const wchar_t*>(
                           (hiddenSrc + "/desktop.ini").toStdWString().c_str()),
                       FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    check(QFileInfo(hiddenSrc + "/desktop.ini").isHidden(),
          "test fixture really is hidden (fixture sanity)");

    FileCopyEngine hiddenEngine;
    hiddenEngine.copyDirectory(hiddenSrc, root + "/hidden-dst");

    check(QFileInfo::exists(root + "/hidden-dst/desktop.ini"),
          "hidden file is copied to destination");
    check(hiddenEngine.verifyDirectory(hiddenSrc, root + "/hidden-dst"),
          "hidden file no longer breaks verification");

    qint64 hiddenSize = 0;
    hiddenEngine.copyDirectory(hiddenSrc, root + "/hidden-dst2",
                               [&](const CopyProgress& p) { hiddenSize = p.totalBytes; });
    qint64 onDiskHidden = 0;
    for (const QString& f : { QString("visible.txt"), QString("desktop.ini"),
                              QString("nested/.secret") }) {
        onDiskHidden += QFileInfo(hiddenSrc + "/" + f).size();
    }
    check(hiddenSize == onDiskHidden,
          QString("size traversal also counts hidden files (%1 vs %2)")
              .arg(hiddenSize).arg(onDiskHidden));

    SetFileAttributesW(reinterpret_cast<const wchar_t*>(
                           (hiddenSrc + "/desktop.ini").toStdWString().c_str()),
                       FILE_ATTRIBUTE_NORMAL);
    SetFileAttributesW(reinterpret_cast<const wchar_t*>(hiddenSrc.toStdWString().c_str()),
                       FILE_ATTRIBUTE_NORMAL);

    // --- error paths ---
    out << "-- error paths --\n";
    check(engine.copyDirectory(root + "/nope", root + "/dstX") == CopyResult::ErrorSourceNotFound,
          "missing source directory reports ErrorSourceNotFound");

    out << "=== " << (g_checks - g_failures) << "/" << g_checks << " checks passed ===\n";

    if (g_failures > 0) {
        out << "RESULT: FAIL (" << g_failures << " failing)\n";
        return 1;
    }

    out << "RESULT: PASS\n";
    return 0;
}