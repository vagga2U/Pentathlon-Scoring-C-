#pragma once

#include "eventmodel.h"

#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

class FinalResultsTab : public QWidget
{
    Q_OBJECT

public:
    explicit FinalResultsTab(QWidget *parent = nullptr);

    void setDivisionSelection(const DivisionSelection &selection);
    void populate(const QVector<Competitor> &rankedCompetitors, const EventModel &eventModel, const QList<DisciplineType> &selectedDisciplines);

signals:
    void divisionSelectionChanged(const DivisionSelection &selection);
    void printRequested();

private slots:
    void emitDivisionSelection();

private:
    ResultDivision currentDivisionFromCombo(const QComboBox *combo) const;
    void populateDivisionCombo(QComboBox *combo);

    QComboBox *m_primaryCombo = nullptr;
    QComboBox *m_secondaryCombo = nullptr;
    QTableWidget *m_table = nullptr;
};
