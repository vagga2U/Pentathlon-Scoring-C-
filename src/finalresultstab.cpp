#include "finalresultstab.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>

namespace
{
class SortableTableItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;

    bool operator<(const QTableWidgetItem &other) const override
    {
        const QVariant leftSortKey = data(Qt::UserRole + 1);
        const QVariant rightSortKey = other.data(Qt::UserRole + 1);
        if (leftSortKey.isValid() && rightSortKey.isValid())
        {
            if (leftSortKey.typeId() == QMetaType::Int && rightSortKey.typeId() == QMetaType::Int)
            {
                return leftSortKey.toInt() < rightSortKey.toInt();
            }
            return leftSortKey.toString() < rightSortKey.toString();
        }
        return QTableWidgetItem::operator<(other);
    }
};

SortableTableItem *sortableItem(const QString &text, const QVariant &sortKey = QVariant())
{
    auto *item = new SortableTableItem(text);
    if (sortKey.isValid())
    {
        item->setData(Qt::UserRole + 1, sortKey);
    }
    return item;
}
}

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
    m_table->setSortingEnabled(true);

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
    const int sortColumn = m_table->horizontalHeader()->sortIndicatorSection();
    const Qt::SortOrder sortOrder = m_table->horizontalHeader()->sortIndicatorOrder();
    const bool sortingWasEnabled = m_table->isSortingEnabled();
    if (sortingWasEnabled)
    {
        m_table->setSortingEnabled(false);
    }

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
        const QString primaryGroup = eventModel.divisionKeyForCompetitor(competitor, currentDivisionFromCombo(m_primaryCombo));
        const QString secondaryGroup = eventModel.divisionKeyForCompetitor(competitor, currentDivisionFromCombo(m_secondaryCombo));

        m_table->setItem(row, 0, sortableItem(QString::number(rank), rank));
        m_table->setItem(row, 1, sortableItem(primaryGroup, primaryGroup.toLower()));
        m_table->setItem(row, 2, sortableItem(secondaryGroup, secondaryGroup.toLower()));
        m_table->setItem(row, 3, sortableItem(competitor.displayName(), competitor.displayName().toLower()));
        m_table->setItem(row, 4, sortableItem(competitor.gender, competitor.gender.toLower()));
        m_table->setItem(row, 5, sortableItem(competitor.category, competitor.category.toLower()));

        int column = 6;
        for (const DisciplineType discipline : disciplines)
        {
            const int score = competitor.scores.value(discipline);
            m_table->setItem(row, column++, sortableItem(disciplineScoreDisplayValue(competitor, discipline), score));
        }
        const int total = eventModel.competitorTotalScore(competitor);
        m_table->setItem(row, column, sortableItem(QString::number(total), total));
        ++rank;
    }

    if (sortingWasEnabled)
    {
        m_table->setSortingEnabled(true);
        if (sortColumn >= 0 && sortColumn < m_table->columnCount())
        {
            m_table->sortItems(sortColumn, sortOrder);
        }
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
