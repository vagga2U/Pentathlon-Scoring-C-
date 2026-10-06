#pragma once

#include "eventmodel.h"

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QWidget>

class FencingGroupTab : public QWidget
{
    Q_OBJECT

public:
    explicit FencingGroupTab(QWidget *parent = nullptr);

    void populate(const QVector<Competitor> &competitors);
    void setStatusMessage(const QString &message, bool isError);

signals:
    void autoAssignGroupsRequested(int numberPistes);

private:
    static QString groupLabelForIndex(int groupIndex);

    QSpinBox *m_pistesSpin = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTreeWidget *m_groupTree = nullptr;
};
