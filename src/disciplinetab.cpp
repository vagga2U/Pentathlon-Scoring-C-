#include "disciplinetab.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

DisciplineTab::DisciplineTab(DisciplineType discipline, QWidget *parent)
    : QWidget(parent)
    , m_discipline(discipline)
{
    auto *layout = new QVBoxLayout(this);

    m_ruleSummaryLabel = new QLabel(this);
    m_ruleSummaryLabel->setWordWrap(true);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);

    m_lanesSpin = new QSpinBox(this);
    m_lanesSpin->setRange(1, 24);
    m_lanesSpin->setValue(8);

    auto *assignButton = new QPushButton("Assign Heats/Lanes", this);
    auto *recalculateButton = new QPushButton("Calculate Scores", this);
    auto *blankButton = new QPushButton("Print Blank Sheet", this);
    auto *resultsButton = new QPushButton("Print Phase Results", this);

    toolbarLayout->addWidget(new QLabel("Lanes / slots per heat", this));
    toolbarLayout->addWidget(m_lanesSpin);
    toolbarLayout->addWidget(assignButton);
    toolbarLayout->addWidget(recalculateButton);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(blankButton);
    toolbarLayout->addWidget(resultsButton);
    m_table = new QTableWidget(this);
    configureColumns();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addWidget(m_ruleSummaryLabel);
    layout->addWidget(toolbar);
    layout->addWidget(m_table, 1);

    connect(assignButton, &QPushButton::clicked, this, [this]() {
        emit assignHeatsRequested(m_discipline, m_lanesSpin->value());
    });
    connect(recalculateButton, &QPushButton::clicked, this, [this]() {
        emit recalculateRequested(m_discipline);
    });
    connect(blankButton, &QPushButton::clicked, this, [this]() {
        emit printBlankRequested(m_discipline);
    });
    connect(resultsButton, &QPushButton::clicked, this, [this]() {
        emit printResultsRequested(m_discipline);
    });
    connect(m_table, &QTableWidget::cellChanged, this, &DisciplineTab::handleCellChanged);
}

DisciplineType DisciplineTab::discipline() const
{
    return m_discipline;
}

void DisciplineTab::setRuleSummary(const DisciplineRule *rule)
{
    if (rule == nullptr)
    {
        m_ruleSummaryLabel->setText("No scoring rule loaded for this discipline.");
        return;
    }

    QString mode;
    switch (rule->scoringKind)
    {
    case ScoringKind::Manual:
        mode = "Manual score entry";
        break;
    case ScoringKind::LinearTime:
        mode = QString("Linear time scoring (%1, ref %2 pts)").arg(rule->measurementHint).arg(rule->referencePoints);
        break;
    case ScoringKind::PlacementTable:
        mode = "Placement table scoring";
        break;
    }

    m_ruleSummaryLabel->setText(QString("%1 input: %2. %3")
        .arg(disciplineDisplayName(m_discipline), rule->inputLabel, mode));
}

void DisciplineTab::populate(const QVector<Competitor> &competitors)
{
    m_updating = true;
    configureColumns();
    m_rowToCompetitorIndex.clear();
    for (int index = 0; index < competitors.size(); ++index)
    {
        if (competitorParticipatesInDiscipline(competitors.at(index), m_discipline))
        {
            m_rowToCompetitorIndex.push_back(index);
        }
    }

    m_table->setRowCount(m_rowToCompetitorIndex.size());

    for (int row = 0; row < m_rowToCompetitorIndex.size(); ++row)
    {
        const Competitor &competitor = competitors.at(m_rowToCompetitorIndex.at(row));
        const LaneAssignment assignment = competitor.laneAssignments.value(m_discipline);
        const QString rawResult = competitor.rawResults.value(m_discipline);
        const QString penalty = competitor.penalties.value(m_discipline);
        const QString score = disciplineScoreDisplayValue(competitor, m_discipline);

        m_table->setItem(row, 0, new QTableWidgetItem(assignment.heat > 0 ? QString::number(assignment.heat) : ""));
        m_table->setItem(row, 1, new QTableWidgetItem(assignment.lane > 0 ? QString::number(assignment.lane) : ""));
        m_table->setItem(row, 2, new QTableWidgetItem(competitor.bib));
        m_table->setItem(row, 3, new QTableWidgetItem(competitor.displayName()));
        m_table->setItem(row, 4, new QTableWidgetItem(competitor.gender));
        m_table->setItem(row, 5, new QTableWidgetItem(competitor.category));
        m_table->setItem(row, 6, new QTableWidgetItem(rawResult));
        m_table->setItem(row, 7, new QTableWidgetItem(penalty));
        m_table->setItem(row, 8, new QTableWidgetItem(score));
    }

    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        for (int column = 0; column < m_table->columnCount(); ++column)
        {
            QTableWidgetItem *item = m_table->item(row, column);
            if (item == nullptr)
            {
                continue;
            }

            Qt::ItemFlags flags = item->flags();
            const bool isHeatLaneColumn = (column == 0 || column == 1);
            const bool isTimeColumn = (column == 6);
            const bool isPenaltyColumn = (column == 7);
            if (isHeatLaneColumn || isTimeColumn || isPenaltyColumn)
            {
                item->setFlags(flags | Qt::ItemIsEditable);
            }
            else
            {
                item->setFlags(flags & ~Qt::ItemIsEditable);
            }
        }
    }

    m_updating = false;
}

void DisciplineTab::handleCellChanged(int row, int column)
{
    if (m_updating)
    {
        return;
    }

    if (row < 0 || row >= m_rowToCompetitorIndex.size())
    {
        return;
    }

    const int competitorIndex = m_rowToCompetitorIndex.at(row);

    if (column == 0 || column == 1)
    {
        bool heatOk = false;
        bool laneOk = false;
        const QString heatText = m_table->item(row, 0) != nullptr ? m_table->item(row, 0)->text().trimmed() : QString();
        const QString laneText = m_table->item(row, 1) != nullptr ? m_table->item(row, 1)->text().trimmed() : QString();
        const int heat = heatText.isEmpty() ? 0 : heatText.toInt(&heatOk);
        const int lane = laneText.isEmpty() ? 0 : laneText.toInt(&laneOk);
        emit laneAssignmentChanged(m_discipline, competitorIndex, (heatText.isEmpty() || heatOk) ? heat : 0, (laneText.isEmpty() || laneOk) ? lane : 0);
        return;
    }

    if (column == 6)
    {
        QTableWidgetItem *item = m_table->item(row, column);
        emit rawResultChanged(m_discipline, competitorIndex, item != nullptr ? item->text() : QString());
        return;
    }

    if (column == 7)
    {
        QTableWidgetItem *item = m_table->item(row, column);
        emit penaltyChanged(m_discipline, competitorIndex, item != nullptr ? item->text() : QString());
    }
}

void DisciplineTab::configureColumns()
{
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({"Heat", "Lane", "Number", "Name", "Gender", "Category", "Time", "Penalty", "Score"});
}

bool DisciplineTab::isLaserRun() const
{
    return m_discipline == DisciplineType::LaserRun;
}
