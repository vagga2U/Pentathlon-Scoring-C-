#include "finalresultstab.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>

FinalResultsTab::FinalResultsTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);

    m_primaryCombo = new QComboBox(this);
    m_secondaryCombo = new QComboBox(this);
    populateDivisionCombo(m_primaryCombo);
    populateDivisionCombo(m_secondaryCombo);

    auto *printButton = new QPushButton("Print Final Results", this);

    toolbarLayout->addWidget(new QLabel("Primary split", this));
    toolbarLayout->addWidget(m_primaryCombo);
    toolbarLayout->addWidget(new QLabel("Secondary split", this));
    toolbarLayout->addWidget(m_secondaryCombo);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(printButton);

    m_table = new QTableWidget(this);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(toolbar);
    layout->addWidget(m_table, 1);

    connect(m_primaryCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &FinalResultsTab::emitDivisionSelection);
    connect(m_secondaryCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &FinalResultsTab::emitDivisionSelection);
    connect(printButton, &QPushButton::clicked, this, &FinalResultsTab::printRequested);
}

void FinalResultsTab::setDivisionSelection(const DivisionSelection &selection)
{
    m_primaryCombo->setCurrentIndex(m_primaryCombo->findData(static_cast<int>(selection.primary)));
    m_secondaryCombo->setCurrentIndex(m_secondaryCombo->findData(static_cast<int>(selection.secondary)));
}

void FinalResultsTab::populate(const QVector<Competitor> &rankedCompetitors, const EventModel &eventModel, const QList<DisciplineType> &selectedDisciplines)
{
    const QList<DisciplineType> disciplines = selectedDisciplines.isEmpty()
        ? allDisciplines()
        : selectedDisciplines;

    QStringList headers = {"Rank", "Primary Group", "Secondary Group", "Name", "Gender", "Category"};
    for (const DisciplineType discipline : disciplines)
    {
        headers.push_back(QString("%1 Score").arg(disciplineDisplayName(discipline)));
    }
    headers.push_back("Total");

    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setRowCount(rankedCompetitors.size());

    int rank = 1;
    for (int row = 0; row < rankedCompetitors.size(); ++row)
    {
        const Competitor &competitor = rankedCompetitors.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(rank++)));
        m_table->setItem(row, 1, new QTableWidgetItem(eventModel.divisionKeyForCompetitor(competitor, currentDivisionFromCombo(m_primaryCombo))));
        m_table->setItem(row, 2, new QTableWidgetItem(eventModel.divisionKeyForCompetitor(competitor, currentDivisionFromCombo(m_secondaryCombo))));
        m_table->setItem(row, 3, new QTableWidgetItem(competitor.displayName()));
        m_table->setItem(row, 4, new QTableWidgetItem(competitor.gender));
        m_table->setItem(row, 5, new QTableWidgetItem(competitor.category));

        int column = 6;
        for (const DisciplineType discipline : disciplines)
        {
            m_table->setItem(row, column++, new QTableWidgetItem(disciplineScoreDisplayValue(competitor, discipline)));
        }
        m_table->setItem(row, column, new QTableWidgetItem(QString::number(eventModel.competitorTotalScore(competitor))));
    }
}

void FinalResultsTab::emitDivisionSelection()
{
    DivisionSelection selection;
    selection.primary = currentDivisionFromCombo(m_primaryCombo);
    selection.secondary = currentDivisionFromCombo(m_secondaryCombo);
    emit divisionSelectionChanged(selection);
}

ResultDivision FinalResultsTab::currentDivisionFromCombo(const QComboBox *combo) const
{
    return static_cast<ResultDivision>(combo->currentData().toInt());
}

void FinalResultsTab::populateDivisionCombo(QComboBox *combo)
{
    combo->addItem(resultDivisionDisplayName(ResultDivision::Overall), static_cast<int>(ResultDivision::Overall));
    combo->addItem(resultDivisionDisplayName(ResultDivision::AgeCategory), static_cast<int>(ResultDivision::AgeCategory));
    combo->addItem(resultDivisionDisplayName(ResultDivision::Gender), static_cast<int>(ResultDivision::Gender));
    combo->addItem(resultDivisionDisplayName(ResultDivision::Affiliation), static_cast<int>(ResultDivision::Affiliation));
}
