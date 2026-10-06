#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>

class ConflictDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConflictDialog(const QString& fileName, QWidget* parent = nullptr);

    bool applyToAll() const;
    int action() const;

private:
    int m_action = 0;
    QCheckBox* m_applyToAll;
};
