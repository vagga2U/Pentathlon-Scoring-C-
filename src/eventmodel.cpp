#include "eventmodel.h"

#include <algorithm>
#include <QSet>
#include <QRegularExpression>
#include <QStringList>

namespace
{
QStringList allowedCategories()
{
    return {
        "U9",
        "U11",
        "U13",
        "U15",
        "U17",
        "U19",
        "Juniors",
        "Seniors",
        "Masters 30+",
        "Masters 40+",
        "Masters 50+",
        "Masters 60+",
        "Masters 70+"
    };
}

QString canonicalCategory(const QString &category)
{
    const QString trimmed = category.trimmed();
    if (trimmed.isEmpty())
    {
        return {};
    }

    for (const QString &allowed : allowedCategories())
    {
        if (trimmed.compare(allowed, Qt::CaseInsensitive) == 0)
        {
            return allowed;
        }
    }
    return {};
}

QString categorySortKey(const QString &category)
{
    const QString normalizedCategory = canonicalCategory(category);
    const QString normalized = normalizedCategory.isEmpty() ? category.trimmed().toLower() : normalizedCategory.toLower();
    if (normalized.isEmpty())
    {
        return "9-999-";
    }

    const QRegularExpression youthPattern("^u\\s*(\\d+)");
    QRegularExpressionMatch match = youthPattern.match(normalized);
    if (match.hasMatch())
    {
        const int age = match.captured(1).toInt();
        return QString("0-%1-%2").arg(age, 3, 10, QChar('0')).arg(normalized);
    }

    if (normalized.contains("junior"))
    {
        return QString("1-019-%1").arg(normalized);
    }
    if (normalized.contains("senior"))
    {
        return QString("1-022-%1").arg(normalized);
    }

    const QRegularExpression mastersPattern("(\\d+)\\s*\\+");
    match = mastersPattern.match(normalized);
    if (match.hasMatch())
    {
        const int age = match.captured(1).toInt();
        return QString("2-%1-%2").arg(age, 3, 10, QChar('0')).arg(normalized);
    }

    const QRegularExpression numberPattern("(\\d+)");
    match = numberPattern.match(normalized);
    if (match.hasMatch())
    {
        const int age = match.captured(1).toInt();
        return QString("1-%1-%2").arg(age, 3, 10, QChar('0')).arg(normalized);
    }

    return QString("8-999-%1").arg(normalized);
}

QString categoryFromBirthYear(int birthYear, int competitionYear)
{
    if (birthYear <= 0 || competitionYear <= 0 || birthYear > competitionYear)
    {
        return "Uncategorised";
    }

    const int age = competitionYear - birthYear;
    if (age <= 8)
    {
        return "U9";
    }
    if (age <= 10)
    {
        return "U11";
    }
    if (age <= 12)
    {
        return "U13";
    }
    if (age <= 14)
    {
        return "U15";
    }
    if (age <= 16)
    {
        return "U17";
    }
    if (age <= 18)
    {
        return "U19";
    }
    if (age <= 21)
    {
        return "Juniors";
    }
    if (age <= 29)
    {
        return "Seniors";
    }
    if (age <= 39)
    {
        return "Masters 30+";
    }
    if (age <= 49)
    {
        return "Masters 40+";
    }
    if (age <= 59)
    {
        return "Masters 50+";
    }
    if (age <= 69)
    {
        return "Masters 60+";
    }
    return "Masters 70+";
}

void applyCategory(Competitor &competitor, int competitionYear)
{
    const QString autoCategory = categoryFromBirthYear(competitor.birthYear, competitionYear);

    if (competitor.categoryOverride)
    {
        const QString canonicalOverride = canonicalCategory(competitor.category);
        if (canonicalOverride.isEmpty() || canonicalOverride.compare(autoCategory, Qt::CaseInsensitive) == 0)
        {
            competitor.categoryOverride = false;
            competitor.category = autoCategory;
        }
        else
        {
            competitor.category = canonicalOverride;
        }
        return;
    }

    competitor.category = autoCategory;
}

bool compareDivision(const EventModel *model, const Competitor &left, const Competitor &right, ResultDivision division)
{
    if (division == ResultDivision::AgeCategory)
    {
        const QString leftCategoryKey = categorySortKey(left.category);
        const QString rightCategoryKey = categorySortKey(right.category);
        if (leftCategoryKey != rightCategoryKey)
        {
            return leftCategoryKey < rightCategoryKey;
        }
    }

    const QString leftKey = model->divisionKeyForCompetitor(left, division).toLower();
    const QString rightKey = model->divisionKeyForCompetitor(right, division).toLower();
    if (leftKey != rightKey)
    {
        return leftKey < rightKey;
    }

    return false;
}
}

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

bool competitorParticipatesInDiscipline(const Competitor &competitor, DisciplineType discipline)
{
    return competitor.disciplineParticipation.value(discipline, true);
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
    recalculateCategoriesForEventYear();
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
    recalculateCategoriesForEventYear();
    emit changed();
}

void EventModel::addCompetitor(const Competitor &competitor)
{
    Competitor updated = competitor;
    applyCategory(updated, m_eventDetails.eventDate.year());
    m_competitors.push_back(updated);
    emit changed();
}

void EventModel::updateCompetitor(int competitorIndex, const Competitor &competitor)
{
    if (competitorIndex < 0 || competitorIndex >= m_competitors.size())
    {
        return;
    }

    Competitor updated = competitor;
    applyCategory(updated, m_eventDetails.eventDate.year());
    m_competitors[competitorIndex] = updated;
    emit changed();
}

void EventModel::assignSequentialHeatsAndLanes(DisciplineType discipline, int lanesPerHeat)
{
    if (lanesPerHeat <= 0)
    {
        return;
    }

    int assignmentIndex = 0;
    for (int index = 0; index < m_competitors.size(); ++index)
    {
        if (!competitorParticipatesInDiscipline(m_competitors.at(index), discipline))
        {
            m_competitors[index].laneAssignments.remove(discipline);
            continue;
        }

        LaneAssignment assignment;
        assignment.heat = (assignmentIndex / lanesPerHeat) + 1;
        assignment.lane = (assignmentIndex % lanesPerHeat) + 1;
        m_competitors[index].laneAssignments.insert(discipline, assignment);
        ++assignmentIndex;
    }

    emit changed();
}

QString EventModel::autoAssignFencingGroups(int numberPistes)
{
    if (numberPistes <= 0)
    {
        return "Number of pistes must be at least 1.";
    }

    QVector<int> fencingIndexes;
    fencingIndexes.reserve(m_competitors.size());
    for (int index = 0; index < m_competitors.size(); ++index)
    {
        if (competitorParticipatesInDiscipline(m_competitors.at(index), DisciplineType::Fencing))
        {
            fencingIndexes.push_back(index);
        }
    }

    const int competitorCount = fencingIndexes.size();
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
    competitorIndexes = fencingIndexes;

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

QString EventModel::autoAssignBibNumbers(const QVector<int> &competitorIndexesInOrder, int startingNumber)
{
    if (startingNumber <= 0)
    {
        return "Starting number must be greater than zero.";
    }

    if (competitorIndexesInOrder.size() != m_competitors.size())
    {
        return "Could not auto assign numbers because the current view does not include all competitors.";
    }

    QSet<int> uniqueIndexes;
    for (const int competitorIndex : competitorIndexesInOrder)
    {
        if (competitorIndex < 0 || competitorIndex >= m_competitors.size())
        {
            return "Could not auto assign numbers because one or more rows map to an invalid competitor.";
        }
        uniqueIndexes.insert(competitorIndex);
    }

    if (uniqueIndexes.size() != competitorIndexesInOrder.size())
    {
        return "Could not auto assign numbers because duplicate rows were detected in the current view.";
    }

    for (int orderIndex = 0; orderIndex < competitorIndexesInOrder.size(); ++orderIndex)
    {
        const int competitorIndex = competitorIndexesInOrder.at(orderIndex);
        m_competitors[competitorIndex].bib = QString::number(startingNumber + orderIndex);
    }

    emit changed();
    return {};
}

void EventModel::recalculateCategoriesForEventYear()
{
    const int competitionYear = m_eventDetails.eventDate.year();
    for (Competitor &competitor : m_competitors)
    {
        applyCategory(competitor, competitionYear);
    }
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
        if (compareDivision(this, left, right, selection.primary))
        {
            return true;
        }
        if (compareDivision(this, right, left, selection.primary))
        {
            return false;
        }

        if (compareDivision(this, left, right, selection.secondary))
        {
            return true;
        }
        if (compareDivision(this, right, left, selection.secondary))
        {
            return false;
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
