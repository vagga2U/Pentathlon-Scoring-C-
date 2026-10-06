#include "ruleset.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>

namespace
{
ScoringKind scoringKindFromKey(const QString &key)
{
    const QString normalized = key.trimmed().toLower();
    if (normalized == "lineartime")
    {
        return ScoringKind::LinearTime;
    }
    if (normalized == "placementtable")
    {
        return ScoringKind::PlacementTable;
    }
    return ScoringKind::Manual;
}

TimeRounding timeRoundingFromKey(const QString &key)
{
    const QString normalized = key.trimmed().toLower();
    if (normalized == "downtosecond")
    {
        return TimeRounding::DownToSecond;
    }
    if (normalized == "downtotwotenths")
    {
        return TimeRounding::DownToTwoTenths;
    }
    if (normalized == "downtohalfsecond")
    {
        return TimeRounding::DownToHalfSecond;
    }
    if (normalized == "downtothirdstep")
    {
        return TimeRounding::DownToThirdStep;
    }
    return TimeRounding::None;
}

QString normalizeScoringKey(const QString &value)
{
    QString normalized = value.trimmed().toLower();
    normalized.remove('_');
    normalized.remove(' ');
    return normalized;
}

int applyRoundingForRule(int milliseconds, TimeRounding rounding)
{
    switch (rounding)
    {
    case TimeRounding::None:
        return milliseconds;
    case TimeRounding::DownToSecond:
        return (milliseconds / 1000) * 1000;
    case TimeRounding::DownToTwoTenths:
        return (milliseconds / 200) * 200;
    case TimeRounding::DownToHalfSecond:
        return (milliseconds / 500) * 500;
    case TimeRounding::DownToThirdStep:
    {
        const int secondsPart = milliseconds / 1000;
        const int millisecondRemainder = milliseconds % 1000;
        if (millisecondRemainder < 330)
        {
            return secondsPart * 1000;
        }
        if (millisecondRemainder < 660)
        {
            return secondsPart * 1000 + 330;
        }
        return secondsPart * 1000 + 660;
    }
    }

    return milliseconds;
}

Ruleset parseRulesetFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
    {
        return {};
    }

    const QJsonObject root = document.object();

    Ruleset ruleset;
    ruleset.id = root.value("id").toString();
    ruleset.name = root.value("name").toString();
    ruleset.description = root.value("description").toString();

    const QJsonArray disciplines = root.value("disciplines").toArray();
    for (const QJsonValue &disciplineValue : disciplines)
    {
        if (!disciplineValue.isObject())
        {
            continue;
        }

        const QJsonObject disciplineObject = disciplineValue.toObject();
        const auto discipline = disciplineFromKey(disciplineObject.value("discipline").toString());
        if (!discipline.has_value())
        {
            continue;
        }

        DisciplineRule rule;
        rule.discipline = *discipline;
        rule.inputLabel = disciplineObject.value("inputLabel").toString("Result");
        rule.measurementHint = disciplineObject.value("measurementHint").toString();
        rule.scoringKind = scoringKindFromKey(normalizeScoringKey(disciplineObject.value("scoringKind").toString()));
        rule.referenceMilliseconds = disciplineObject.value("referenceMilliseconds").toInt();
        rule.referencePoints = disciplineObject.value("referencePoints").toInt();
        rule.millisecondsPerPoint = disciplineObject.value("millisecondsPerPoint").toInt(1000);
        rule.higherIsBetter = disciplineObject.value("higherIsBetter").toBool(false);
        rule.minimumPoints = disciplineObject.value("minimumPoints").toInt();
        rule.maximumPoints = disciplineObject.value("maximumPoints").toInt(-1);
        rule.timeRounding = timeRoundingFromKey(normalizeScoringKey(disciplineObject.value("timeRounding").toString()));

        const QJsonArray placementPoints = disciplineObject.value("placementPoints").toArray();
        for (const QJsonValue &placementValue : placementPoints)
        {
            const QJsonObject placementObject = placementValue.toObject();
            rule.placementPoints.insert(placementObject.value("place").toInt(), placementObject.value("points").toInt());
        }

        ruleset.disciplineRules.push_back(rule);
    }

    return ruleset;
}
}

const DisciplineRule *Ruleset::ruleFor(DisciplineType discipline) const
{
    for (const DisciplineRule &rule : disciplineRules)
    {
        if (rule.discipline == discipline)
        {
            return &rule;
        }
    }

    return nullptr;
}

QVector<Ruleset> RulesetRepository::loadFromDirectory(const QString &directoryPath) const
{
    QVector<Ruleset> rulesets;

    QDir directory(directoryPath);
    const QStringList files = directory.entryList({"*.json"}, QDir::Files | QDir::Readable, QDir::Name);
    for (const QString &fileName : files)
    {
        const Ruleset ruleset = parseRulesetFile(directory.filePath(fileName));
        if (!ruleset.id.isEmpty())
        {
            rulesets.push_back(ruleset);
        }
    }

    if (rulesets.isEmpty())
    {
        rulesets.push_back(fallbackRuleset());
    }

    return rulesets;
}

Ruleset RulesetRepository::fallbackRuleset() const
{
    Ruleset ruleset;
    ruleset.id = "manual-template";
    ruleset.name = "Manual Scoring Template";
    ruleset.description = "Fallback ruleset when no JSON rule packs are found.";

    for (const DisciplineType discipline : allDisciplines())
    {
        DisciplineRule rule;
        rule.discipline = discipline;
        rule.inputLabel = "Points";
        rule.measurementHint = "Manual score entry";
        rule.scoringKind = ScoringKind::Manual;
        ruleset.disciplineRules.push_back(rule);
    }

    return ruleset;
}

std::optional<int> parseDurationToMilliseconds(const QString &value)
{
    QString normalized = value.trimmed();
    normalized.replace(',', '.');
    if (normalized.isEmpty())
    {
        return std::nullopt;
    }

    bool ok = false;
    const int rawMilliseconds = normalized.toInt(&ok);
    if (ok)
    {
        return rawMilliseconds;
    }

    const QStringList minuteSections = normalized.split(':');
    QString secondsSection;
    int minutes = 0;

    if (minuteSections.size() == 1)
    {
        secondsSection = minuteSections.front();
    }
    else if (minuteSections.size() == 2)
    {
        minutes = minuteSections.front().toInt(&ok);
        if (!ok)
        {
            return std::nullopt;
        }
        secondsSection = minuteSections.back();
    }
    else
    {
        return std::nullopt;
    }

    const QStringList secondSections = secondsSection.split('.');
    const int seconds = secondSections.front().toInt(&ok);
    if (!ok)
    {
        return std::nullopt;
    }

    int hundredths = 0;
    if (secondSections.size() > 1)
    {
        QString decimals = secondSections.at(1).leftJustified(2, '0').left(2);
        hundredths = decimals.toInt(&ok);
        if (!ok)
        {
            return std::nullopt;
        }
    }

    return ((minutes * 60) + seconds) * 1000 + (hundredths * 10);
}

std::optional<int> scoreRawResult(const DisciplineRule &rule, const QString &rawResult)
{
    const QString trimmed = rawResult.trimmed();
    if (trimmed.isEmpty())
    {
        return std::nullopt;
    }

    bool ok = false;
    switch (rule.scoringKind)
    {
    case ScoringKind::Manual:
    {
        const int value = trimmed.toInt(&ok);
        return ok ? std::optional<int>(value) : std::nullopt;
    }

    case ScoringKind::LinearTime:
    {
        const auto milliseconds = parseDurationToMilliseconds(trimmed);
        if (!milliseconds.has_value() || rule.millisecondsPerPoint <= 0)
        {
            return std::nullopt;
        }

        const int roundedMilliseconds = applyRoundingForRule(*milliseconds, rule.timeRounding);

        const int delta = rule.higherIsBetter
            ? (roundedMilliseconds - rule.referenceMilliseconds)
            : (rule.referenceMilliseconds - roundedMilliseconds);
        const int adjustment = static_cast<int>(std::floor(static_cast<double>(delta) / static_cast<double>(rule.millisecondsPerPoint)));
        return rule.referencePoints + adjustment;
    }

    case ScoringKind::PlacementTable:
    {
        const int place = trimmed.toInt(&ok);
        if (!ok)
        {
            return std::nullopt;
        }
        if (rule.placementPoints.contains(place))
        {
            return rule.placementPoints.value(place);
        }
        return rule.minimumPoints;
    }
    }

    return std::nullopt;
}
