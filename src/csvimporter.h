#pragma once

#include "eventmodel.h"

#include <QString>
#include <QStringList>

struct CsvImportResult
{
    QVector<Competitor> competitors;
    QStringList warnings;
    bool bonusRoundEnabled = false;
    QMap<QString, int> bonusRoundPlacingsByBib;
};

class CsvImporter
{
public:
    CsvImportResult importEntries(const QString &filePath) const;
};
