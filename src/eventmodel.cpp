#include "eventmodel.h"

#include <algorithm>

QString Competitor::displayName() const
{
    return QString("%1 %2").arg(firstName.trimmed(), lastName.trimmed()).trimmed();
}

int Competitor::totalScore() const
{
    int total = 0;
    for (auto it = scores.cbegin(); it != scores.cend(); ++it)
    {
        total += it.value();
    }
    return total;
}

QString resultDivisionDisplayName(ResultDivision division)
{
    switch (division)
    {
    case ResultDivision::Overall:
        return "Overall";
    case ResultDivision::AgeCategory:
        return "Age Category";
    case ResultDivision::Gender:
        return "Gender";
    case ResultDivision::Affiliation:
        return "Affiliation";
    }

    return "Overall";
}

QString disciplineScoreDisplayValue(const Competitor &competitor, DisciplineType discipline)
{
    const QString penalty = competitor.penalties.value(discipline).trimmed().toUpper();
    if (penalty == "DSQ" || penalty == "ELM")
    {
        return penalty;
    }

    if (competitor.scores.contains(discipline))
    {
        return QString::number(competitor.scores.value(discipline));
    }

    return {};
}

EventModel::EventModel(QObject *parent)
    : QObject(parent)
{
}

const EventDetails &EventModel::eventDetails() const
{
    return m_eventDetails;
}

const DivisionSelection &EventModel::divisionSelection() const
{
    return m_divisionSelection;
}

const QVector<Competitor> &EventModel::competitors() const
{
    return m_competitors;
}

void EventModel::setEventDetails(const EventDetails &details)
{
    m_eventDetails = details;
    emit changed();
}

void EventModel::setDivisionSelection(const DivisionSelection &selection)
{
    m_divisionSelection = selection;
    emit changed();
}

void EventModel::setCompetitors(const QVector<Competitor> &competitors)
{
    m_competitors = competitors;
    emit changed();
}

void EventModel::addCompetitor(const Competitor &competitor)
{
    m_competitors.push_back(competitor);
    emit changed();
}

void EventModel::assignSequentialHeatsAndLanes(DisciplineType discipline, int lanesPerHeat)
{
    if (lanesPerHeat <= 0)
    {
        return;
    }

    for (int index = 0; index < m_competitors.size(); ++index)
    {
        LaneAssignment assignment;
        assignment.heat = (index / lanesPerHeat) + 1;
        assignment.lane = (index % lanesPerHeat) + 1;
        m_competitors[index].laneAssignments.insert(discipline, assignment);
    }

    emit changed();
}

QString EventModel::autoAssignFencingGroups(int numberPistes)
{
    if (numberPistes <= 0)
    {
        return "Number of pistes must be at least 1.";
    }

    const int competitorCount = m_competitors.size();
    const int maxGroups = (2 * numberPistes) + 1;
    const int maxCompetitors = maxGroups * 4;
    if (competitorCount > maxCompetitors)
    {
        return QString("Insufficient pistes available. %1 competitors exceed the limit of %2 for %3 piste(s).")
            .arg(competitorCount)
            .arg(maxCompetitors)
            .arg(numberPistes);
    }

    for (Competitor &competitor : m_competitors)
    {
        competitor.fencingGroup = 0;
    }

    if (competitorCount == 0)
    {
        emit changed();
        return {};
    }

    QVector<int> targetGroupSizes;
    if (competitorCount <= maxGroups * 2)
    {
        const int targetGroups = (competitorCount + 1) / 2;
        targetGroupSizes.fill(2, targetGroups);
        if (competitorCount % 2 != 0)
        {
            targetGroupSizes[targetGroups - 1] = 1;
        }
    }
    else if (competitorCount <= (2 * numberPistes) * 3)
    {
        const int targetGroups = 2 * numberPistes;
        const int number3Groups = competitorCount - (targetGroups * 2);
        targetGroupSizes.fill(2, targetGroups);
        for (int index = 0; index < number3Groups; ++index)
        {
            targetGroupSizes[index] = 3;
        }
    }
    else if (competitorCount <= maxGroups * 3)
    {
        const int targetGroups = maxGroups;
        const int number3Groups = competitorCount - (targetGroups * 2);
        targetGroupSizes.fill(2, targetGroups);
        for (int index = 0; index < number3Groups; ++index)
        {
            targetGroupSizes[index] = 3;
        }
    }
    else if (competitorCount <= (2 * numberPistes) * 4)
    {
        const int targetGroups = 2 * numberPistes;
        const int number4Groups = competitorCount - (targetGroups * 3);
        targetGroupSizes.fill(3, targetGroups);
        for (int index = 0; index < number4Groups; ++index)
        {
            targetGroupSizes[index] = 4;
        }
    }
    else
    {
        const int targetGroups = maxGroups;
        const int number4Groups = competitorCount - (targetGroups * 3);
        targetGroupSizes.fill(3, targetGroups);
        for (int index = 0; index < number4Groups; ++index)
        {
            targetGroupSizes[index] = 4;
        }
    }

    QVector<int> competitorIndexes;
    competitorIndexes.reserve(competitorCount);
    for (int index = 0; index < competitorCount; ++index)
    {
        competitorIndexes.push_back(index);
    }

    std::sort(competitorIndexes.begin(), competitorIndexes.end(), [this](int leftIndex, int rightIndex) {
        const Competitor &left = m_competitors.at(leftIndex);
        const Competitor &right = m_competitors.at(rightIndex);

        const QString leftAffiliation = left.affiliation.trimmed().toLower();
        const QString rightAffiliation = right.affiliation.trimmed().toLower();
        if (leftAffiliation != rightAffiliation)
        {
            if (leftAffiliation.isEmpty())
            {
                return false;
            }
            if (rightAffiliation.isEmpty())
            {
                return true;
            }
            return leftAffiliation < rightAffiliation;
        }

        const QString leftName = left.displayName().toLower();
        const QString rightName = right.displayName().toLower();
        if (leftName != rightName)
        {
            return leftName < rightName;
        }

        return left.bib.toLower() < right.bib.toLower();
    });

    int assignedCount = 0;
    int offset = 0;
    for (int groupNumber = 1; groupNumber <= targetGroupSizes.size(); ++groupNumber)
    {
        const int groupSize = targetGroupSizes.at(groupNumber - 1);
        for (int seat = 0; seat < groupSize && offset < competitorIndexes.size(); ++seat)
        {
            const int competitorIndex = competitorIndexes.at(offset++);
            m_competitors[competitorIndex].fencingGroup = groupNumber;
            ++assignedCount;
        }
    }

    if (assignedCount != competitorCount)
    {
        return "Could not assign all competitors to fencing groups.";
    }

    emit changed();
    return {};
}

void EventModel::updateLaneAssignment(int competitorIndex, DisciplineType discipline, int heat, int lane)
{
    if (competitorIndex < 0 || competitorIndex >= m_competitors.size())
    {
        return;
    }

    LaneAssignment assignment;
    assignment.heat = std::max(0, heat);
    assignment.lane = std::max(0, lane);
    m_competitors[competitorIndex].laneAssignments.insert(discipline, assignment);
    emit changed();
}

void EventModel::updateRawResult(int competitorIndex, DisciplineType discipline, const QString &rawResult)
{
    if (competitorIndex < 0 || competitorIndex >= m_competitors.size())
    {
        return;
    }

    m_competitors[competitorIndex].rawResults.insert(discipline, rawResult.trimmed());
    emit changed();
}

void EventModel::updatePenalty(int competitorIndex, DisciplineType discipline, const QString &penalty)
{
    if (competitorIndex < 0 || competitorIndex >= m_competitors.size())
    {
        return;
    }

    m_competitors[competitorIndex].penalties.insert(discipline, penalty.trimmed());
    emit changed();
}

void EventModel::updateScore(int competitorIndex, DisciplineType discipline, int score)
{
    if (competitorIndex < 0 || competitorIndex >= m_competitors.size())
    {
        return;
    }

    m_competitors[competitorIndex].scores.insert(discipline, score);
    emit changed();
}

QVector<Competitor> EventModel::rankedCompetitors() const
{
    return rankedCompetitors(m_divisionSelection);
}

QVector<Competitor> EventModel::rankedCompetitors(const DivisionSelection &selection) const
{
    QVector<Competitor> ranked = m_competitors;
    std::sort(ranked.begin(), ranked.end(), [this, &selection](const Competitor &left, const Competitor &right) {
        const QString leftPrimary = divisionKeyForCompetitor(left, selection.primary).toLower();
        const QString rightPrimary = divisionKeyForCompetitor(right, selection.primary).toLower();
        if (leftPrimary != rightPrimary)
        {
            return leftPrimary < rightPrimary;
        }

        const QString leftSecondary = divisionKeyForCompetitor(left, selection.secondary).toLower();
        const QString rightSecondary = divisionKeyForCompetitor(right, selection.secondary).toLower();
        if (leftSecondary != rightSecondary)
        {
            return leftSecondary < rightSecondary;
        }

        const int leftTotal = competitorTotalScore(left);
        const int rightTotal = competitorTotalScore(right);
        if (leftTotal == rightTotal)
        {
            return left.displayName().toLower() < right.displayName().toLower();
        }
        return leftTotal > rightTotal;
    });
    return ranked;
}

QString EventModel::divisionKeyForCompetitor(const Competitor &competitor, ResultDivision division) const
{
    switch (division)
    {
    case ResultDivision::Overall:
        return "Overall";
    case ResultDivision::AgeCategory:
        return competitor.category.trimmed().isEmpty() ? "Uncategorised" : competitor.category.trimmed();
    case ResultDivision::Gender:
        return competitor.gender.trimmed().isEmpty() ? "Unspecified" : competitor.gender.trimmed();
    case ResultDivision::Affiliation:
        return competitor.affiliation.trimmed().isEmpty() ? "No Affiliation" : competitor.affiliation.trimmed();
    }

    return "Overall";
}

int EventModel::competitorTotalScore(const Competitor &competitor) const
{
    const QList<DisciplineType> selectedDisciplines = m_eventDetails.selectedDisciplines.isEmpty()
        ? allDisciplines()
        : m_eventDetails.selectedDisciplines;

    int total = 0;
    for (const DisciplineType discipline : selectedDisciplines)
    {
        total += competitor.scores.value(discipline);
    }
    return total;
}
