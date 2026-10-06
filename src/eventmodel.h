#pragma once

#include "disciplines.h"

#include <QDate>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>

struct LaneAssignment
{
    int heat = 0;
    int lane = 0;
};

struct Competitor
{
    QString bib;
    QString firstName;
    QString lastName;
    QString gender;
    QString category;
    bool categoryOverride = false;
    QString affiliation;
    int birthYear = 0;
    int fencingGroup = 0;
    QMap<DisciplineType, bool> disciplineParticipation;
    QMap<DisciplineType, LaneAssignment> laneAssignments;
    QMap<DisciplineType, QString> rawResults;
    QMap<DisciplineType, QString> penalties;
    QMap<DisciplineType, int> scores;

    QString displayName() const;
    int totalScore() const;
};

struct EventDetails
{
    QString eventName;
    QDate eventDate = QDate::currentDate();
    QString logoPath;
    QString csvPath;
    QString rulesetId;
    QList<DisciplineType> selectedDisciplines = allDisciplines();
};

enum class ResultDivision
{
    Overall,
    AgeCategory,
    Gender,
    Affiliation
};

QString resultDivisionDisplayName(ResultDivision division);
QString disciplineScoreDisplayValue(const Competitor &competitor, DisciplineType discipline);
bool competitorParticipatesInDiscipline(const Competitor &competitor, DisciplineType discipline);

struct DivisionSelection
{
    ResultDivision primary = ResultDivision::Overall;
    ResultDivision secondary = ResultDivision::Overall;
};

class EventModel : public QObject
{
    Q_OBJECT

public:
    explicit EventModel(QObject *parent = nullptr);

    const EventDetails &eventDetails() const;
    const DivisionSelection &divisionSelection() const;
    const QVector<Competitor> &competitors() const;

    void setEventDetails(const EventDetails &details);
    void setDivisionSelection(const DivisionSelection &selection);
    void setCompetitors(const QVector<Competitor> &competitors);
    void addCompetitor(const Competitor &competitor);
    void updateCompetitor(int competitorIndex, const Competitor &competitor);
    void assignSequentialHeatsAndLanes(DisciplineType discipline, int lanesPerHeat);
    QString autoAssignFencingGroups(int numberPistes);
    QString autoAssignBibNumbers(const QVector<int> &competitorIndexesInOrder, int startingNumber);
    void recalculateCategoriesForEventYear();
    void updateLaneAssignment(int competitorIndex, DisciplineType discipline, int heat, int lane);
    void updateRawResult(int competitorIndex, DisciplineType discipline, const QString &rawResult);
    void updatePenalty(int competitorIndex, DisciplineType discipline, const QString &penalty);
    void updateScore(int competitorIndex, DisciplineType discipline, int score);

    QVector<Competitor> rankedCompetitors() const;
    QVector<Competitor> rankedCompetitors(const DivisionSelection &selection) const;
    QString divisionKeyForCompetitor(const Competitor &competitor, ResultDivision division) const;
    int competitorTotalScore(const Competitor &competitor) const;

signals:
    void changed();

private:
    EventDetails m_eventDetails;
    DivisionSelection m_divisionSelection;
    QVector<Competitor> m_competitors;
};
