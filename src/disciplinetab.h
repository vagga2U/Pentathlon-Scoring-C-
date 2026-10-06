#pragma once

#include "eventmodel.h"
#include "ruleset.h"

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

class DisciplineTab : public QWidget
{
    Q_OBJECT

public:
    explicit DisciplineTab(DisciplineType discipline, QWidget *parent = nullptr);

    DisciplineType discipline() const;
    void setRuleSummary(const DisciplineRule *rule);
    void populate(const QVector<Competitor> &competitors);

signals:
    void assignHeatsRequested(DisciplineType discipline, int lanesPerHeat);
    void laneAssignmentChanged(DisciplineType discipline, int competitorIndex, int heat, int lane);
    void rawResultChanged(DisciplineType discipline, int competitorIndex, const QString &rawResult);
    void penaltyChanged(DisciplineType discipline, int competitorIndex, const QString &penalty);
    void recalculateRequested(DisciplineType discipline);
    void printBlankRequested(DisciplineType discipline);
    void printResultsRequested(DisciplineType discipline);

private slots:
    void handleCellChanged(int row, int column);

private:
    void configureColumns();
    bool isLaserRun() const;

    DisciplineType m_discipline;
    QLabel *m_ruleSummaryLabel = nullptr;
    QSpinBox *m_lanesSpin = nullptr;
    QTableWidget *m_table = nullptr;
    bool m_updating = false;
};
