#pragma once

#include <QString>
#include <QStringList>
#include <QFileInfo>

namespace Utils {

inline QString formatSpeed(double bps)
{
    if (bps >= 1024.0 * 1024.0 * 1024.0)
        return QString("%1 GB/s").arg(bps / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    if (bps >= 1024.0 * 1024.0)
        return QString("%1 MB/s").arg(bps / (1024.0 * 1024.0), 0, 'f', 2);
    if (bps >= 1024.0)
        return QString("%1 KB/s").arg(bps / 1024.0, 0, 'f', 2);
    return QString("%1 B/s").arg(static_cast<int>(bps));
}

inline QString formatEta(double seconds)
{
    if (seconds < 60)
        return QString("%1s").arg(static_cast<int>(seconds));
    if (seconds < 3600)
        return QString("%1m %2s").arg(static_cast<int>(seconds) / 60).arg(static_cast<int>(seconds) % 60);
    return QString("%1h %2m").arg(static_cast<int>(seconds) / 3600).arg((static_cast<int>(seconds) % 3600) / 60);
}

inline QString cleanPath(const QString& path)
{
    QString p = path;
    while (p.endsWith('/') || p.endsWith('\\'))
        p.chop(1);
    return p;
}

inline bool isSubPath(const QString& parent, const QString& child)
{
    return child.startsWith(parent + "/") || child.startsWith(parent + "\\");
}

inline QString normalizedPath(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return cleanPath(path.toLower());
}

// True when two paths name the same location, or one is an ancestor of the
// other ("E:/" vs "E:/CODE"). Windows paths compare case-insensitively and
// both separators are valid. Copying across overlapping paths either
// truncates the source in place (dest == source file) or recurses into the
// copy while it is being created (dest inside source), so both directions
// must be refused up front.
inline bool pathsOverlap(const QString& a, const QString& b)
{
    const QString x = normalizedPath(a);
    const QString y = normalizedPath(b);
    return x == y || isSubPath(x, y) || isSubPath(y, x);
}

}
