#pragma once

#include "eventmodel.h"

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTreeWidget>
#include <QWidget>

struct FencingRoundScheduleRow
{
    int round = 0;
    int piste = 0;
    QString matchup;
    QString bouts;
};

class FencingGroupTab : public QWidget
{
    Q_OBJECT

public:
    explicit FencingGroupTab(QWidget *parent = nullptr);

    void populate(const QVector<Competitor> &competitors);
    void setStatusMessage(const QString &message, bool isError);
    int estimatedRuntimeMinutes() const;
    int estimatedRoundCount() const;
    int maxBoutsPerPistePerRound() const;
    QVector<FencingRoundScheduleRow> roundScheduleRows() const;

signals:
    void autoAssignGroupsRequested(int numberPistes);
    void exportGroupListRequested();
    void exportRoundSheetsRequested();

private:
    static QString groupLabelForIndex(int groupIndex);

    QSpinBox *m_pistesSpin = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTreeWidget *m_groupTree = nullptr;
    QTableWidget *m_roundsTable = nullptr;
    int m_estimatedRuntimeMinutes = 0;
    int m_estimatedRoundCount = 0;
    int m_maxBoutsPerPistePerRound = 0;
    QVector<FencingRoundScheduleRow> m_roundScheduleRows;
};
