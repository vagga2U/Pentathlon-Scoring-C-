#pragma once

#include "disciplines.h"

#include <QMap>
#include <QString>
#include <QVector>

#include <optional>

enum class ScoringKind
{
    Manual,
    LinearTime,
    PlacementTable
};

enum class TimeRounding
{
    None,
    DownToSecond,
    DownToTwoTenths,
    DownToHalfSecond,
    DownToThirdStep
};

struct DisciplineRule
{
    DisciplineType discipline = DisciplineType::Fencing;
    QString inputLabel;
    QString measurementHint;
    ScoringKind scoringKind = ScoringKind::Manual;
    int referenceMilliseconds = 0;
    int referencePoints = 0;
    int millisecondsPerPoint = 1000;
    bool higherIsBetter = false;
    int minimumPoints = 0;
    int maximumPoints = -1;
    TimeRounding timeRounding = TimeRounding::None;
    QMap<int, int> placementPoints;
};

struct Ruleset
{
    QString id;
    QString name;
    QString description;
    QVector<DisciplineRule> disciplineRules;

    const DisciplineRule *ruleFor(DisciplineType discipline) const;
};

class RulesetRepository
{
public:
    QVector<Ruleset> loadFromDirectory(const QString &directoryPath) const;
    Ruleset fallbackRuleset() const;
};

std::optional<int> parseDurationToMilliseconds(const QString &value);
std::optional<int> scoreRawResult(const DisciplineRule &rule, const QString &rawResult);
