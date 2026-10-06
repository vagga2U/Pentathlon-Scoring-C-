#include "fencinggrouptab.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QSet>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
QVector<QPair<int, int>> roundZeroBoutOrder(int groupSize)
{
    if (groupSize == 2)
    {
        return {{1, 2}};
    }
    if (groupSize == 3)
    {
        return {{1, 3}, {2, 1}, {3, 2}};
    }
    if (groupSize == 4)
    {
        return {{1, 3}, {2, 4}, {2, 1}, {4, 3}, {1, 4}, {3, 2}};
    }
    return {};
}

QVector<QPair<int, int>> interGroupTemplateForNominalSize(int nominalSize)
{
    if (nominalSize <= 2)
    {
        return {{1, 3}, {1, 4}, {2, 4}, {2, 3}};
    }
    if (nominalSize == 3)
    {
        return {{1, 4}, {2, 5}, {3, 6}, {5, 1}, {6, 2}, {4, 3}, {1, 6}, {2, 4}, {3, 5}};
    }
    return {{3, 8}, {4, 6}, {1, 7}, {2, 5}, {6, 3}, {8, 1}, {5, 4}, {7, 2}, {1, 6}, {3, 5}, {2, 8}, {4, 7}, {5, 1}, {6, 2}, {7, 3}, {8, 4}};
}

QString intraGroupBoutList(const QString &groupLabel, int groupSize)
{
    const QVector<QPair<int, int>> order = roundZeroBoutOrder(groupSize);
    QStringList bouts;
    bouts.reserve(order.size());
    for (const QPair<int, int> &bout : order)
    {
        bouts.push_back(QString("%1%2-%3%4").arg(groupLabel).arg(bout.first).arg(groupLabel).arg(bout.second));
    }
    return bouts.join(", ");
}

QString interGroupBoutList(const QString &leftGroupLabel, int leftSize, const QString &rightGroupLabel, int rightSize)
{
    const int nominalSize = std::max(2, std::min(4, std::max(leftSize, rightSize)));
    const QVector<QPair<int, int>> templateOrder = interGroupTemplateForNominalSize(nominalSize);
    QStringList bouts;
    for (const QPair<int, int> &templateBout : templateOrder)
    {
        auto descriptorForNumber = [nominalSize, &leftGroupLabel, leftSize, &rightGroupLabel, rightSize](int number, QString *descriptor) {
            const bool isLeft = number <= nominalSize;
            const int slot = isLeft ? number : (number - nominalSize);
            if ((isLeft && slot > leftSize) || (!isLeft && slot > rightSize))
            {
                return false;
            }

            *descriptor = QString("%1%2").arg(isLeft ? leftGroupLabel : rightGroupLabel).arg(slot);
            return true;
        };

        QString leftDescriptor;
        QString rightDescriptor;
        if (!descriptorForNumber(templateBout.first, &leftDescriptor) || !descriptorForNumber(templateBout.second, &rightDescriptor))
        {
            continue;
        }
        bouts.push_back(QString("%1-%2").arg(leftDescriptor, rightDescriptor));
    }
    return bouts.join(", ");
}

QVector<QPair<int, int>> roundRobinPairsForRound(const QVector<int> &teams, int roundIndex)
{
    QVector<int> orderedTeams = teams;
    std::sort(orderedTeams.begin(), orderedTeams.end());
    const int byeTeam = -1;
    const bool hasOddTeamCount = (orderedTeams.size() % 2 != 0);
    if (hasOddTeamCount)
    {
        orderedTeams.push_back(byeTeam);
    }

    const int totalTeams = orderedTeams.size();
    const int rowSize = totalTeams / 2;
    if (rowSize <= 0)
    {
        return {};
    }

    QVector<int> topRow;
    QVector<int> bottomRow;
    topRow.reserve(rowSize);
    bottomRow.reserve(rowSize);

    QVector<int> rotatingTeams = orderedTeams;
    if (hasOddTeamCount)
    {
        topRow.push_back(byeTeam);
        rotatingTeams.removeOne(byeTeam);
    }
    else
    {
        topRow.push_back(orderedTeams.front());
        rotatingTeams = orderedTeams.mid(1);
    }

    for (int index = 0; index < rowSize - 1; ++index)
    {
        topRow.push_back(rotatingTeams.at(index));
    }

    QVector<int> remaining = rotatingTeams.mid(rowSize - 1);
    std::reverse(remaining.begin(), remaining.end());
    bottomRow = remaining;

    for (int step = 0; step < roundIndex; ++step)
    {
        const int topRowLast = topRow.back();
        const int bottomRowFirst = bottomRow.front();

        for (int index = rowSize - 1; index >= 2; --index)
        {
            topRow[index] = topRow[index - 1];
        }
        topRow[1] = bottomRowFirst;

        for (int index = 0; index < rowSize - 1; ++index)
        {
            bottomRow[index] = bottomRow[index + 1];
        }
        bottomRow[rowSize - 1] = topRowLast;
    }

    QVector<QPair<int, int>> pairs;
    for (int index = 0; index < rowSize; ++index)
    {
        pairs.push_back({topRow.at(index), bottomRow.at(index)});
    }
    return pairs;
}
}

FencingGroupTab::FencingGroupTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);

    m_pistesSpin = new QSpinBox(this);
    m_pistesSpin->setRange(1, 20);
    m_pistesSpin->setValue(1);

    auto *assignButton = new QPushButton("Auto Assign Groups", this);
    auto *exportButton = new QPushButton("Export Group List (PDF)...", this);
    auto *exportRoundSheetsButton = new QPushButton("Export Fencing Round Sheets (PDF)...", this);
    toolbarLayout->addWidget(new QLabel("Number of pistes", this));
    toolbarLayout->addWidget(m_pistesSpin);
    toolbarLayout->addWidget(assignButton);
    toolbarLayout->addWidget(exportButton);
    toolbarLayout->addWidget(exportRoundSheetsButton);
    toolbarLayout->addStretch();

    m_statusLabel = new QLabel("Enter number of pistes and click Auto Assign Groups.", this);
    m_statusLabel->setWordWrap(true);

    m_groupTree = new QTreeWidget(this);
    m_groupTree->setColumnCount(3);
    m_groupTree->setHeaderLabels({"Number", "Name", "Affiliation"});
    m_groupTree->header()->setSectionResizeMode(QHeaderView::Stretch);

    m_roundsTable = new QTableWidget(this);
    m_roundsTable->setColumnCount(4);
    m_roundsTable->setHorizontalHeaderLabels({"Round", "Piste", "Matchup", "Bouts"});
    m_roundsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_roundsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(toolbar);
    layout->addWidget(m_statusLabel);
    layout->addWidget(new QLabel("Assigned Groups", this));
    layout->addWidget(m_groupTree, 1);
    layout->addWidget(new QLabel("Ranking Round Bout Order", this));
    layout->addWidget(m_roundsTable, 1);

    connect(assignButton, &QPushButton::clicked, this, [this]() {
        emit autoAssignGroupsRequested(m_pistesSpin->value());
    });
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        emit exportGroupListRequested();
    });
    connect(exportRoundSheetsButton, &QPushButton::clicked, this, [this]() {
        emit exportRoundSheetsRequested();
    });
}

void FencingGroupTab::populate(const QVector<Competitor> &competitors)
{
    m_groupTree->clear();
    m_roundsTable->clearContents();
    m_roundsTable->setRowCount(0);
    m_roundScheduleRows.clear();
    m_estimatedRuntimeMinutes = 0;
    m_estimatedRoundCount = 0;
    m_maxBoutsPerPistePerRound = 0;

    QMap<int, QVector<const Competitor *>> groups;
    QVector<const Competitor *> unassigned;
    for (const Competitor &competitor : competitors)
    {
        if (!competitorParticipatesInDiscipline(competitor, DisciplineType::Fencing))
        {
            continue;
        }

        if (competitor.fencingGroup > 0)
        {
            groups[competitor.fencingGroup].push_back(&competitor);
        }
        else
        {
            unassigned.push_back(&competitor);
        }
    }

    for (auto it = groups.begin(); it != groups.end(); ++it)
    {
        QVector<const Competitor *> sorted = it.value();
        std::sort(sorted.begin(), sorted.end(), [](const Competitor *left, const Competitor *right) {
            const QString leftName = left->displayName().toLower();
            const QString rightName = right->displayName().toLower();
            if (leftName != rightName)
            {
                return leftName < rightName;
            }
            return left->bib.toLower() < right->bib.toLower();
        });

        const QString groupTitle = QString("Group %1 (%2)")
            .arg(groupLabelForIndex(it.key()))
            .arg(sorted.size() == 1 ? "1 fencer" : QString("%1 fencers").arg(sorted.size()));
        auto *groupItem = new QTreeWidgetItem(QStringList() << groupTitle);
        m_groupTree->addTopLevelItem(groupItem);

        for (const Competitor *competitor : sorted)
        {
            auto *item = new QTreeWidgetItem(groupItem);
            item->setText(0, competitor->bib);
            item->setText(1, competitor->displayName());
            item->setText(2, competitor->affiliation.trimmed().isEmpty() ? "No affiliation" : competitor->affiliation);
        }
    }

    if (!unassigned.isEmpty())
    {
        std::sort(unassigned.begin(), unassigned.end(), [](const Competitor *left, const Competitor *right) {
            const QString leftName = left->displayName().toLower();
            const QString rightName = right->displayName().toLower();
            if (leftName != rightName)
            {
                return leftName < rightName;
            }
            return left->bib.toLower() < right->bib.toLower();
        });

        auto *groupItem = new QTreeWidgetItem(QStringList() << QString("Unassigned (%1)").arg(unassigned.size()));
        m_groupTree->addTopLevelItem(groupItem);

        for (const Competitor *competitor : unassigned)
        {
            auto *item = new QTreeWidgetItem(groupItem);
            item->setText(0, competitor->bib);
            item->setText(1, competitor->displayName());
            item->setText(2, competitor->affiliation.trimmed().isEmpty() ? "No affiliation" : competitor->affiliation);
        }
    }

    m_groupTree->expandAll();

    if (groups.isEmpty())
    {
        return;
    }

    QMap<int, int> groupSizes;
    QVector<int> groupNumbers;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it)
    {
        groupNumbers.push_back(it.key());
        groupSizes.insert(it.key(), it.value().size());
    }

    QVector<int> roundRobinTeams = groupNumbers;
    const int byeTeam = -1;

    QVector<FencingRoundScheduleRow> rows;
    rows.reserve(groups.size() * 6);
    QMap<int, int> firstPisteByGroup;
    const int effectiveTeamCount = (roundRobinTeams.size() % 2 == 0)
        ? roundRobinTeams.size()
        : (roundRobinTeams.size() + 1);
    const int roundCount = std::max(0, effectiveTeamCount - 1);
    for (int roundIndex = 0; roundIndex < roundCount; ++roundIndex)
    {
        const QVector<QPair<int, int>> roundPairs = roundRobinPairsForRound(roundRobinTeams, roundIndex);
        int piste = 1;
        for (const QPair<int, int> &pair : roundPairs)
        {
            const int leftGroup = pair.first;
            const int rightGroup = pair.second;
            if (leftGroup == byeTeam || rightGroup == byeTeam)
            {
                continue;
            }

            if (!firstPisteByGroup.contains(leftGroup))
            {
                firstPisteByGroup.insert(leftGroup, piste);
            }
            if (!firstPisteByGroup.contains(rightGroup))
            {
                firstPisteByGroup.insert(rightGroup, piste);
            }

            const QString leftLabel = groupLabelForIndex(leftGroup);
            const QString rightLabel = groupLabelForIndex(rightGroup);

            FencingRoundScheduleRow roundRow;
            roundRow.round = roundIndex + 1;
            roundRow.piste = piste;
            roundRow.matchup = QString("%1 vs %2").arg(leftLabel, rightLabel);
            roundRow.bouts = interGroupBoutList(leftLabel, groupSizes.value(leftGroup), rightLabel, groupSizes.value(rightGroup));
            if (!roundRow.bouts.isEmpty())
            {
                rows.push_back(roundRow);
            }

            ++piste;
        }
    }

    for (const int groupNumber : groupNumbers)
    {
        const QString label = groupLabelForIndex(groupNumber);
        const QString bouts = intraGroupBoutList(label, groupSizes.value(groupNumber));
        if (bouts.isEmpty())
        {
            continue;
        }

        FencingRoundScheduleRow roundZero;
        roundZero.round = 0;
        roundZero.piste = firstPisteByGroup.value(groupNumber, 1);
        roundZero.matchup = QString("%1 vs %1").arg(label);
        roundZero.bouts = bouts;
        rows.push_back(roundZero);
    }

    std::sort(rows.begin(), rows.end(), [](const FencingRoundScheduleRow &left, const FencingRoundScheduleRow &right) {
        if (left.round != right.round)
        {
            return left.round < right.round;
        }
        if (left.piste != right.piste)
        {
            return left.piste < right.piste;
        }
        return left.matchup < right.matchup;
    });

    QSet<int> roundsWithBouts;
    int maxBoutsPerPistePerRound = 0;
    for (const FencingRoundScheduleRow &roundRow : rows)
    {
        const QStringList boutEntries = roundRow.bouts.split(',', Qt::SkipEmptyParts);
        maxBoutsPerPistePerRound = std::max(maxBoutsPerPistePerRound, static_cast<int>(boutEntries.size()));
        roundsWithBouts.insert(roundRow.round);
    }

    m_estimatedRoundCount = roundsWithBouts.size();
    m_maxBoutsPerPistePerRound = maxBoutsPerPistePerRound;
    if (m_estimatedRoundCount > 0 && m_maxBoutsPerPistePerRound > 0)
    {
        m_estimatedRuntimeMinutes =
            (2 * m_estimatedRoundCount * m_maxBoutsPerPistePerRound) + m_estimatedRoundCount;
    }

    m_roundScheduleRows = rows;

    m_roundsTable->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row)
    {
        const FencingRoundScheduleRow &roundRow = rows.at(row);
        m_roundsTable->setItem(row, 0, new QTableWidgetItem(roundRow.round == 0 ? "0 (Intra Group)" : QString::number(roundRow.round)));
        m_roundsTable->setItem(row, 1, new QTableWidgetItem(QString::number(roundRow.piste)));
        m_roundsTable->setItem(row, 2, new QTableWidgetItem(roundRow.matchup));
        m_roundsTable->setItem(row, 3, new QTableWidgetItem(roundRow.bouts));
    }
}

void FencingGroupTab::setStatusMessage(const QString &message, bool isError)
{
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(isError ? "color: #ff6666;" : QString());
}

int FencingGroupTab::estimatedRuntimeMinutes() const
{
    return m_estimatedRuntimeMinutes;
}

int FencingGroupTab::estimatedRoundCount() const
{
    return m_estimatedRoundCount;
}

int FencingGroupTab::maxBoutsPerPistePerRound() const
{
    return m_maxBoutsPerPistePerRound;
}

QVector<FencingRoundScheduleRow> FencingGroupTab::roundScheduleRows() const
{
    return m_roundScheduleRows;
}

QString FencingGroupTab::groupLabelForIndex(int groupIndex)
{
    if (groupIndex <= 0)
    {
        return "?";
    }

    QString label;
    int value = groupIndex;
    while (value > 0)
    {
        const int remainder = (value - 1) % 26;
        label.prepend(QChar(static_cast<char>('A' + remainder)));
        value = (value - 1) / 26;
    }
    return label;
}
