#pragma once

#include "eventmodel.h"

#include <QString>
#include <QStringList>

struct CsvImportResult
{
    QVector<Competitor> competitors;
    QStringList warnings;
};

class CsvImporter
{
public:
    CsvImportResult importEntries(const QString &filePath) const;
};
