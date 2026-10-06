#include "ConflictDialog.h"
#include "../common/Theme.h"

ConflictDialog::ConflictDialog(const QString& fileName, QWidget* parent) : QDialog(parent)
{
    setWindowTitle("File Conflict");
    setModal(true);

    auto* layout = new QVBoxLayout(this);

    auto* msgLabel = new QLabel(QString("'%1' already exists at the destination.").arg(fileName), this);
    msgLabel->setWordWrap(true);

    auto* btnLayout = new QHBoxLayout();
    auto* skipBtn = new QPushButton("Skip", this);
    auto* overwriteBtn = new QPushButton("Overwrite", this);
    auto* renameBtn = new QPushButton("Rename", this);
    overwriteBtn->setObjectName("primaryButton");

    btnLayout->addWidget(skipBtn);
    btnLayout->addWidget(overwriteBtn);
    btnLayout->addWidget(renameBtn);

    m_applyToAll = new QCheckBox("Apply to all conflicts", this);

    layout->addWidget(msgLabel);
    layout->addLayout(btnLayout);
    layout->addWidget(m_applyToAll);

    connect(skipBtn, &QPushButton::clicked, this, [this]() { m_action = 0; accept(); });
    connect(overwriteBtn, &QPushButton::clicked, this, [this]() { m_action = 1; accept(); });
    connect(renameBtn, &QPushButton::clicked, this, [this]() { m_action = 2; accept(); });
}

bool ConflictDialog::applyToAll() const
{
    return m_applyToAll->isChecked();
}

int ConflictDialog::action() const
{
    return m_action;
}
