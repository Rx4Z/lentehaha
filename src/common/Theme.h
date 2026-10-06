#pragma once

#include <QColor>
#include <QString>
#include <QMap>

class Theme
{
public:
    static Theme& instance();

    QColor background() const;
    QColor surface() const;
    QColor surfaceAlt() const;
    QColor border() const;
    QColor primary() const;
    QColor primaryHover() const;
    QColor success() const;
    QColor warning() const;
    QColor error() const;
    QColor text() const;
    QColor textMuted() const;
    QColor accent() const;

    QString stylesheet() const;

private:
    Theme();
    QMap<QString, QColor> m_colors;
};
