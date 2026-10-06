#include "DestinationPicker.h"
#include "../common/Theme.h"
#include "../common/Utils.h"
#include "../core/DriveManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDir>
#include <QStorageInfo>

DestinationPicker::DestinationPicker(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_edit = new QLineEdit(this);
    m_edit->setReadOnly(true);
    m_edit->setPlaceholderText(QStringLiteral("No destination chosen"));

    m_browseBtn = new QPushButton("Select Directory…", this);
    m_browseBtn->setObjectName("primaryButton");

    m_clearBtn = new QPushButton("Clear", this);

    auto* row = new QHBoxLayout();
    row->setSpacing(8);
    row->addWidget(m_edit, 1);
    row->addWidget(m_browseBtn);
    row->addWidget(m_clearBtn);

    layout->addLayout(row);

    m_meter = new QProgressBar(this);
    m_meter->setObjectName("capacityMeter");
    m_meter->setRange(0, 100);
    m_meter->setValue(0);
    m_meter->setTextVisible(true);
    m_meter->setFormat(QStringLiteral("No destination"));
    m_meter->setEnabled(false);

    m_capacityLabel = new QLabel(this);
    m_capacityLabel->setObjectName("statusLabel");
    m_capacityLabel->setWordWrap(true);

    layout->addWidget(m_meter);
    layout->addWidget(m_capacityLabel);

    connect(m_browseBtn, &QPushButton::clicked, this, &DestinationPicker::onBrowse);
    connect(m_clearBtn, &QPushButton::clicked, this, &DestinationPicker::onClear);
}

QString DestinationPicker::destination() const
{
    return m_edit->text();
}

bool DestinationPicker::isValid() const
{
    const QString path = destination();
    return !path.isEmpty() && DriveManager::pathExists(path);
}

qint64 DestinationPicker::freeBytes() const
{
    const QString path = destination();
    return path.isEmpty() ? -1 : DriveManager::freeSpace(path);
}

void DestinationPicker::setDestination(const QString& path)
{
    if (path.isEmpty()) {
        clearDestination();
        return;
    }

    m_edit->setText(QDir::toNativeSeparators(path));
    refreshCapacity();
    emit destinationChanged(destination(), freeBytes());
}

void DestinationPicker::clearDestination()
{
    m_edit->clear();
    m_meter->setValue(0);
    m_meter->setFormat(QStringLiteral("No destination"));
    m_capacityLabel->clear();
    emit destinationChanged(QString(), -1);
}

void DestinationPicker::onBrowse()
{
    const QString start = isValid() ? destination() : DriveManager::homeDirectory();

    const QString chosen = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Select destination directory"), start);

    if (!chosen.isEmpty())
        setDestination(chosen);
}

void DestinationPicker::onClear()
{
    clearDestination();
}

void DestinationPicker::refreshCapacity()
{
    const QString path = destination();
    if (path.isEmpty())
        return;

    const qint64 free = DriveManager::freeSpace(path);
    const qint64 total = DriveManager::totalSpace(path);

    m_meter->setEnabled(true);

    if (total > 0) {
        const int used = static_cast<int>(((total - free) * 100) / total);
        m_meter->setValue(used);
        m_meter->setFormat(QStringLiteral("%1 free of %2")
                               .arg(DriveManager::formatSize(free),
                                    DriveManager::formatSize(total)));
    } else {
        m_meter->setValue(0);
        m_meter->setFormat(QStringLiteral("%1 free").arg(DriveManager::formatSize(free)));
    }

    m_capacityLabel->setText(QStringLiteral("Destination: %1").arg(path));
}