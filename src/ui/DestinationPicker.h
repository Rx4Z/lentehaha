#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

class DestinationPicker : public QWidget
{
    Q_OBJECT

public:
    explicit DestinationPicker(QWidget* parent = nullptr);

    QString destination() const;
    bool isValid() const;
    qint64 freeBytes() const;

    void setDestination(const QString& path);
    void clearDestination();

signals:
    void destinationChanged(const QString& path, qint64 freeBytes);

private slots:
    void onBrowse();
    void onClear();

private:
    void refreshCapacity();

    QLineEdit* m_edit;
    QPushButton* m_browseBtn;
    QPushButton* m_clearBtn;
    QLabel* m_capacityLabel;
    QProgressBar* m_meter;
};