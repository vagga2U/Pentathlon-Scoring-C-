#include "csvimporter.h"

#include <QFile>
#include <QTextStream>

namespace
{
QString normalizeHeader(const QString &header)
{
    QString normalized = header.trimmed().toLower();
    normalized.remove(' ');
    normalized.remove('_');
    normalized.remove('-');
    return normalized;
}

QStringList parseCsvLine(const QString &line)
{
    QStringList values;
    QString current;
    bool inQuotes = false;

    for (int index = 0; index < line.size(); ++index)
    {
        const QChar character = line.at(index);
        if (character == '"')
        {
            if (inQuotes && index + 1 < line.size() && line.at(index + 1) == '"')
            {
                current.append('"');
                ++index;
            }
            else
            {
                inQuotes = !inQuotes;
            }
            continue;
        }

        if (character == ',' && !inQuotes)
        {
            values.push_back(current.trimmed());
            current.clear();
            continue;
        }

        current.append(character);
    }

    values.push_back(current.trimmed());
    return values;
}

QString valueForHeaders(const QStringList &headers, const QStringList &values, const QStringList &aliases)
{
    for (int index = 0; index < headers.size(); ++index)
    {
        if (!aliases.contains(headers.at(index)))
        {
            continue;
        }

        if (index < values.size())
        {
            return values.at(index).trimmed();
        }
    }

    return {};
}

}

CsvImportResult CsvImporter::importEntries(const QString &filePath) const
{
    CsvImportResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.warnings.push_back(QString("Could not open %1").arg(filePath));
        return result;
    }

    QTextStream stream(&file);
    if (stream.atEnd())
    {
        result.warnings.push_back("CSV file is empty.");
        return result;
    }

    const QStringList rawHeaders = parseCsvLine(stream.readLine());
    QStringList headers;
    for (const QString &header : rawHeaders)
    {
        headers.push_back(normalizeHeader(header));
    }

    int lineNumber = 1;
    while (!stream.atEnd())
    {
        ++lineNumber;
        const QString line = stream.readLine();
        if (line.trimmed().isEmpty())
        {
            continue;
        }

        const QStringList values = parseCsvLine(line);

        Competitor competitor;
        competitor.bib = valueForHeaders(headers, values, {"bib", "number", "id"});
        competitor.firstName = valueForHeaders(headers, values, {"firstname", "givenname"});
        competitor.lastName = valueForHeaders(headers, values, {"lastname", "surname", "familyname"});
        competitor.gender = valueForHeaders(headers, values, {"gender", "sex"});
        competitor.category = valueForHeaders(headers, values, {"category", "agecategory", "class"});
        competitor.affiliation = valueForHeaders(headers, values, {"affiliation", "team", "club", "nation"});

        const QString fullName = valueForHeaders(headers, values, {"name", "competitor"});
        if (competitor.firstName.isEmpty() && competitor.lastName.isEmpty() && !fullName.isEmpty())
        {
            const QStringList parts = fullName.split(' ', Qt::SkipEmptyParts);
            if (!parts.isEmpty())
            {
                competitor.firstName = parts.front();
                competitor.lastName = parts.mid(1).join(' ');
            }
        }

        const QString dobString = valueForHeaders(headers, values, {"dob", "dateofbirth", "birthdate"});
        if (!dobString.isEmpty())
        {
            competitor.dateOfBirth = QDate::fromString(dobString, Qt::ISODate);
            if (!competitor.dateOfBirth.isValid())
            {
                competitor.dateOfBirth = QDate::fromString(dobString, "dd/MM/yyyy");
            }
        }

        if (competitor.displayName().isEmpty())
        {
            result.warnings.push_back(QString("Skipped line %1 because no competitor name was found.").arg(lineNumber));
            continue;
        }

        const QString obstacleTime = valueForHeaders(headers, values, {"ocrtime", "ocr", "obstacletime", "obstacle"});
        if (!obstacleTime.isEmpty())
        {
            competitor.rawResults.insert(DisciplineType::Obstacle, obstacleTime);
        }

        const QString swimTime = valueForHeaders(headers, values, {"swimtime", "swimmingtime", "swim", "swimming"});
        if (!swimTime.isEmpty())
        {
            competitor.rawResults.insert(DisciplineType::Swimming, swimTime);
        }

        QString laserTime = valueForHeaders(headers, values, {"laserruntime", "laserrun", "laseruntime", "laser", "lasertime"});
        const QString lap1 = valueForHeaders(headers, values, {"laserlap1", "lap1"});
        const QString lap2 = valueForHeaders(headers, values, {"laserlap2", "lap2"});
        const QString lap3 = valueForHeaders(headers, values, {"laserlap3", "lap3"});
        const QString lap4 = valueForHeaders(headers, values, {"laserlap4", "lap4"});
        const QString lap5 = valueForHeaders(headers, values, {"laserlap5", "lap5"});
        if (laserTime.isEmpty())
        {
            if (!lap5.isEmpty())
            {
                laserTime = lap5;
            }
            else if (!lap4.isEmpty())
            {
                laserTime = lap4;
            }
            else if (!lap3.isEmpty())
            {
                laserTime = lap3;
            }
            else if (!lap2.isEmpty())
            {
                laserTime = lap2;
            }
            else if (!lap1.isEmpty())
            {
                laserTime = lap1;
            }
        }
        if (!laserTime.isEmpty())
        {
            competitor.rawResults.insert(DisciplineType::LaserRun, laserTime);
        }

        result.competitors.push_back(competitor);
    }

    if (result.competitors.isEmpty())
    {
        result.warnings.push_back("No competitors were imported from the selected CSV.");
    }

    return result;
}
