#pragma once

#include <QList>
#include <QString>

#include <optional>

enum class DisciplineType
{
    Fencing,
    Obstacle,
    Swimming,
    LaserRun
};

inline QList<DisciplineType> allDisciplines()
{
    return {DisciplineType::Fencing, DisciplineType::Obstacle, DisciplineType::Swimming, DisciplineType::LaserRun};
}

inline QString disciplineKey(DisciplineType discipline)
{
    switch (discipline)
    {
    case DisciplineType::Fencing:
        return "fencing";
    case DisciplineType::Obstacle:
        return "obstacle";
    case DisciplineType::Swimming:
        return "swimming";
    case DisciplineType::LaserRun:
        return "laser_run";
    }

    return "unknown";
}

inline QString disciplineDisplayName(DisciplineType discipline)
{
    switch (discipline)
    {
    case DisciplineType::Fencing:
        return "Fencing";
    case DisciplineType::Obstacle:
        return "Obstacle";
    case DisciplineType::Swimming:
        return "Swimming";
    case DisciplineType::LaserRun:
        return "Laser Run";
    }

    return "Unknown";
}

inline std::optional<DisciplineType> disciplineFromKey(const QString &key)
{
    const QString normalized = key.trimmed().toLower();

    if (normalized == "fencing")
    {
        return DisciplineType::Fencing;
    }
    if (normalized == "obstacle" || normalized == "ocr")
    {
        return DisciplineType::Obstacle;
    }
    if (normalized == "swimming")
    {
        return DisciplineType::Swimming;
    }
    if (normalized == "laser_run" || normalized == "laserrun" || normalized == "laser run")
    {
        return DisciplineType::LaserRun;
    }

    return std::nullopt;
}
