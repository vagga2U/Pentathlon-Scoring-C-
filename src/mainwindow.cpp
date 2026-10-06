#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImageReader>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTextStream>
#include <QUrl>
#include <QSet>
#include <QPageLayout>
#include <QPageSize>
#include <QRegularExpression>

#include <algorithm>
#include <cstdlib>

namespace
{
QString htmlEscape(QString value)
{
    value.replace('&', "&amp;");
    value.replace('<', "&lt;");
    value.replace('>', "&gt;");
    value.replace('"', "&quot;");
    return value;
}

QString csvEscape(QString value)
{
    value.replace('"', "\"\"");
    if (value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r'))
    {
        return QString("\"%1\"").arg(value);
    }
    return value;
}

QString scoringTimeFromRaw(DisciplineType discipline, const QString &raw)
{
    if (discipline != DisciplineType::LaserRun)
    {
        return raw.trimmed();
    }

    if (!raw.contains('|'))
    {
        return raw.trimmed();
    }

    QStringList laps = raw.split('|', Qt::KeepEmptyParts);
    while (laps.size() < 5)
    {
        laps.push_back(QString());
    }
    return laps.at(4);
}

int laserLapCountForCategory(const QString &category)
{
    const QString normalized = category.trimmed().toLower();
    if (normalized.contains("u9") || normalized.contains("u11"))
    {
        return 2;
    }
    if (normalized.contains("u13") || normalized.contains("u15"))
    {
        return 3;
    }
    if (normalized.contains("u17"))
    {
        return 4;
    }
    if (normalized.contains("u19") || normalized.contains("junior") || normalized.contains("senior"))
    {
        return 5;
    }
    if (normalized.contains("60+") || normalized.contains("70+") || normalized.contains("80+"))
    {
        return 2;
    }
    if (normalized.contains("30+") || normalized.contains("40+") || normalized.contains("50+"))
    {
        return 3;
    }
    return 5;
}

int laserRunReferenceMillisecondsForCategory(const QString &category)
{
    const QString normalized = category.trimmed().toLower();
    if (normalized.contains("u9") || normalized.contains("u11"))
    {
        return 240000;
    }
    if (normalized.contains("u13"))
    {
        return 320000;
    }
    if (normalized.contains("u15"))
    {
        return 460000;
    }
    if (normalized.contains("u17"))
    {
        return 630000;
    }
    if (normalized.contains("30+") || normalized.contains("40+") || normalized.contains("50+") || normalized.contains("m30") || normalized.contains("m40") || normalized.contains("m50"))
    {
        return 690000;
    }
    if (normalized.contains("60+") || normalized.contains("70+") || normalized.contains("80+") || normalized.contains("m60") || normalized.contains("m70") || normalized.contains("m80"))
    {
        return 420000;
    }
    return 800000;
}

int swimmingReferenceMillisecondsForCategory(const QString &category, const QString &gender)
{
    const QString normalizedCategory = category.trimmed().toLower();
    const QString normalizedGender = gender.trimmed().toLower();

    if (normalizedCategory.contains("u9") || normalizedCategory.contains("u11"))
    {
        return 45000;
    }
    if (normalizedCategory.contains("u13") || normalizedCategory.contains("u15") || normalizedCategory.contains("u17") || normalizedCategory.contains("u19") || normalizedCategory.contains("junior") || normalizedCategory.contains("senior"))
    {
        return 70000;
    }
    if (normalizedCategory.contains("30+") || normalizedCategory.contains("40+") || normalizedCategory.contains("50+") || normalizedCategory.contains("m30") || normalizedCategory.contains("m40") || normalizedCategory.contains("m50"))
    {
        return (normalizedGender == "f" || normalizedGender == "female") ? 90000 : 78000;
    }
    if (normalizedCategory.contains("60+") || normalizedCategory.contains("70+") || normalizedCategory.contains("80+") || normalizedCategory.contains("m60") || normalizedCategory.contains("m70") || normalizedCategory.contains("m80"))
    {
        return (normalizedGender == "f" || normalizedGender == "female") ? 43000 : 38000;
    }
    return 70000;
}

int swimmingMillisecondsPerPointForCategory(const QString &category)
{
    const QString normalized = category.trimmed().toLower();
    if (normalized.contains("u9") || normalized.contains("u11") || normalized.contains("30+") || normalized.contains("40+") || normalized.contains("50+") || normalized.contains("60+") || normalized.contains("70+") || normalized.contains("80+") || normalized.contains("m30") || normalized.contains("m40") || normalized.contains("m50") || normalized.contains("m60") || normalized.contains("m70") || normalized.contains("m80"))
    {
        return 500;
    }
    return 200;
}

TimeRounding swimmingTimeRoundingForCategory(const QString &category)
{
    const QString normalized = category.trimmed().toLower();
    if (normalized.contains("u9") || normalized.contains("u11") || normalized.contains("30+") || normalized.contains("40+") || normalized.contains("50+") || normalized.contains("60+") || normalized.contains("70+") || normalized.contains("80+") || normalized.contains("m30") || normalized.contains("m40") || normalized.contains("m50") || normalized.contains("m60") || normalized.contains("m70") || normalized.contains("m80"))
    {
        return TimeRounding::DownToHalfSecond;
    }
    return TimeRounding::DownToTwoTenths;
}

bool isNoTimeRecorded(const DisciplineRule *rule, const QString &rawResult)
{
    if (rule == nullptr || rule->scoringKind != ScoringKind::LinearTime)
    {
        return false;
    }

    const QString trimmed = rawResult.trimmed();
    if (trimmed.isEmpty())
    {
        return true;
    }

    const auto milliseconds = parseDurationToMilliseconds(trimmed);
    return milliseconds.has_value() && *milliseconds <= 0;
}

bool isDsqPenalty(const QString &penalty)
{
    return penalty.trimmed().toUpper() == "DSQ";
}

bool isElmPenalty(const QString &penalty)
{
    return penalty.trimmed().toUpper() == "ELM";
}

std::optional<int> integerPenaltyValue(const QString &penalty)
{
    bool ok = false;
    const int parsed = penalty.trimmed().toInt(&ok);
    if (!ok)
    {
        return std::nullopt;
    }
    return std::abs(parsed);
}

QMap<QString, int> parseFencingMatrixRowRaw(const QString &raw)
{
    QMap<QString, int> scoresByOpponentBib;
    if (raw.trimmed().isEmpty())
    {
        return scoresByOpponentBib;
    }

    const QJsonDocument document = QJsonDocument::fromJson(raw.toUtf8());
    if (!document.isObject())
    {
        return scoresByOpponentBib;
    }

    const QJsonObject matrix = document.object().value("matrix").toObject();
    for (auto it = matrix.begin(); it != matrix.end(); ++it)
    {
        if (it.value().isDouble())
        {
            scoresByOpponentBib.insert(it.key(), std::max(0, it.value().toInt()));
        }
    }
    return scoresByOpponentBib;
}

struct FencingScoreRule
{
    int bouts = 0;
    int victoryValue = 0;
};

QString existingTablePath(const QStringList &fileNames)
{
    QStringList candidatePaths;
    for (const QString &fileName : fileNames)
    {
        candidatePaths.push_back(QDir::current().absoluteFilePath(fileName));
        candidatePaths.push_back(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QString("../%1").arg(fileName)));
        candidatePaths.push_back(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QString("../../%1").arg(fileName)));
    }

    for (const QString &candidatePath : candidatePaths)
    {
        if (QFileInfo::exists(candidatePath))
        {
            return candidatePath;
        }
    }
    return {};
}

QMap<int, FencingScoreRule> loadFencingScoringTable()
{
    const QString tablePath = existingTablePath({"samples/FencingScoring.csv"});

    QMap<int, FencingScoreRule> rulesByBouts;
    if (tablePath.isEmpty())
    {
        return rulesByBouts;
    }

    QFile file(tablePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return rulesByBouts;
    }

    QTextStream stream(&file);
    bool isFirstRow = true;
    while (!stream.atEnd())
    {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
        {
            continue;
        }

        if (isFirstRow)
        {
            isFirstRow = false;
            if (line.startsWith(QChar(0xFEFF)))
            {
                line.remove(0, 1);
            }
            continue;
        }

        const QStringList columns = line.split(',', Qt::KeepEmptyParts);
        if (columns.size() < 2)
        {
            continue;
        }

        bool boutsOk = false;
        bool valueOk = false;
        const int bouts = columns.at(0).trimmed().toInt(&boutsOk);
        const int victoryValue = columns.at(1).trimmed().toInt(&valueOk);
        if (!boutsOk || !valueOk || bouts <= 0 || victoryValue <= 0)
        {
            continue;
        }

        rulesByBouts.insert(bouts, FencingScoreRule{bouts, victoryValue});
    }

    return rulesByBouts;
}

int fencingLinearScoreFromVictories(int victories, int bouts)
{
    if (victories <= 0 || bouts <= 0)
    {
        return 0;
    }

    static const QMap<int, FencingScoreRule> rulesByBouts = loadFencingScoringTable();
    const int targetVictoriesFor250 = static_cast<int>(std::lround(static_cast<double>(bouts) * 0.7));
    const int denominator = std::max(1, bouts - targetVictoriesFor250);

    int victoryValue = 0;
    if (rulesByBouts.contains(bouts))
    {
        victoryValue = rulesByBouts.value(bouts).victoryValue;
    }
    else
    {
        victoryValue = std::max(1, static_cast<int>(std::lround(50.0 / static_cast<double>(denominator))));
    }

    const int score = 250 + ((victories - targetVictoriesFor250) * victoryValue);
    return std::max(0, score);
}

QMap<int, int> loadBonusRoundScoringTable()
{
    const QString tablePath = existingTablePath({"samples/fencingDE.csv", "samples/FencingDE.csv"});
    QMap<int, int> scoreByRank;
    if (tablePath.isEmpty())
    {
        return scoreByRank;
    }

    QFile file(tablePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return scoreByRank;
    }

    QTextStream stream(&file);
    bool isFirstRow = true;
    while (!stream.atEnd())
    {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
        {
            continue;
        }

        if (isFirstRow)
        {
            isFirstRow = false;
            if (line.startsWith(QChar(0xFEFF)))
            {
                line.remove(0, 1);
            }
            continue;
        }

        const QStringList columns = line.split(',', Qt::KeepEmptyParts);
        if (columns.size() < 2)
        {
            continue;
        }

        bool rankOk = false;
        bool scoreOk = false;
        const int rank = columns.at(0).trimmed().toInt(&rankOk);
        const int score = columns.at(1).trimmed().toInt(&scoreOk);
        if (!rankOk || !scoreOk || rank <= 0)
        {
            continue;
        }
        scoreByRank.insert(rank, score);
    }

    return scoreByRank;
}

int divisionToInt(ResultDivision division)
{
    return static_cast<int>(division);
}

ResultDivision divisionFromInt(int value)
{
    switch (value)
    {
    case 1:
        return ResultDivision::AgeCategory;
    case 2:
        return ResultDivision::Gender;
    case 3:
        return ResultDivision::Affiliation;
    default:
        return ResultDivision::Overall;
    }
}

QJsonObject competitorToJson(const Competitor &competitor)
{
    QJsonObject object;
    object.insert("bib", competitor.bib);
    object.insert("firstName", competitor.firstName);
    object.insert("lastName", competitor.lastName);
    object.insert("gender", competitor.gender);
    object.insert("category", competitor.category);
    object.insert("categoryOverride", competitor.categoryOverride);
    object.insert("affiliation", competitor.affiliation);
    object.insert("birthYear", competitor.birthYear);
    object.insert("fencingGroup", competitor.fencingGroup);

    QJsonObject participationObject;
    for (const DisciplineType discipline : allDisciplines())
    {
        participationObject.insert(disciplineKey(discipline), competitorParticipatesInDiscipline(competitor, discipline));
    }
    object.insert("disciplineParticipation", participationObject);

    QJsonObject lanesObject;
    for (auto it = competitor.laneAssignments.cbegin(); it != competitor.laneAssignments.cend(); ++it)
    {
        QJsonObject laneObject;
        laneObject.insert("heat", it.value().heat);
        laneObject.insert("lane", it.value().lane);
        lanesObject.insert(disciplineKey(it.key()), laneObject);
    }
    object.insert("laneAssignments", lanesObject);

    QJsonObject rawResultsObject;
    for (auto it = competitor.rawResults.cbegin(); it != competitor.rawResults.cend(); ++it)
    {
        rawResultsObject.insert(disciplineKey(it.key()), it.value());
    }
    object.insert("rawResults", rawResultsObject);

    QJsonObject penaltiesObject;
    for (auto it = competitor.penalties.cbegin(); it != competitor.penalties.cend(); ++it)
    {
        penaltiesObject.insert(disciplineKey(it.key()), it.value());
    }
    object.insert("penalties", penaltiesObject);

    QJsonObject scoresObject;
    for (auto it = competitor.scores.cbegin(); it != competitor.scores.cend(); ++it)
    {
        scoresObject.insert(disciplineKey(it.key()), it.value());
    }
    object.insert("scores", scoresObject);

    return object;
}

Competitor competitorFromJson(const QJsonObject &object)
{
    Competitor competitor;
    competitor.bib = object.value("bib").toString();
    competitor.firstName = object.value("firstName").toString();
    competitor.lastName = object.value("lastName").toString();
    competitor.gender = object.value("gender").toString();
    competitor.category = object.value("category").toString();
    competitor.categoryOverride = object.value("categoryOverride").toBool(false);
    competitor.affiliation = object.value("affiliation").toString();
    competitor.birthYear = object.value("birthYear").toInt();
    if (competitor.birthYear <= 0)
    {
        const QDate legacyDob = QDate::fromString(object.value("dateOfBirth").toString(), Qt::ISODate);
        if (legacyDob.isValid())
        {
            competitor.birthYear = legacyDob.year();
        }
    }
    competitor.fencingGroup = object.value("fencingGroup").toInt();

    const QJsonObject participationObject = object.value("disciplineParticipation").toObject();
    for (const DisciplineType discipline : allDisciplines())
    {
        const QString key = disciplineKey(discipline);
        if (participationObject.contains(key))
        {
            competitor.disciplineParticipation.insert(discipline, participationObject.value(key).toBool(true));
        }
    }

    const QJsonObject lanesObject = object.value("laneAssignments").toObject();
    for (auto it = lanesObject.begin(); it != lanesObject.end(); ++it)
    {
        const auto discipline = disciplineFromKey(it.key());
        if (!discipline.has_value() || !it.value().isObject())
        {
            continue;
        }

        const QJsonObject laneObject = it.value().toObject();
        LaneAssignment assignment;
        assignment.heat = laneObject.value("heat").toInt();
        assignment.lane = laneObject.value("lane").toInt();
        competitor.laneAssignments.insert(*discipline, assignment);
    }

    const QJsonObject rawResultsObject = object.value("rawResults").toObject();
    for (auto it = rawResultsObject.begin(); it != rawResultsObject.end(); ++it)
    {
        const auto discipline = disciplineFromKey(it.key());
        if (discipline.has_value())
        {
            competitor.rawResults.insert(*discipline, it.value().toString());
        }
    }

    const QJsonObject penaltiesObject = object.value("penalties").toObject();
    for (auto it = penaltiesObject.begin(); it != penaltiesObject.end(); ++it)
    {
        const auto discipline = disciplineFromKey(it.key());
        if (discipline.has_value())
        {
            competitor.penalties.insert(*discipline, it.value().toString());
        }
    }

    const QJsonObject scoresObject = object.value("scores").toObject();
    for (auto it = scoresObject.begin(); it != scoresObject.end(); ++it)
    {
        const auto discipline = disciplineFromKey(it.key());
        if (discipline.has_value())
        {
            competitor.scores.insert(*discipline, it.value().toInt());
        }
    }

    return competitor;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Modern Pentathlon Event Manager");
    resize(1280, 800);

    m_rulesets = m_rulesetRepository.loadFromDirectory(QDir(QCoreApplication::applicationDirPath()).filePath("rulesets"));

    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    m_setupTab = new SetupTab(this);
    m_finalResultsTab = new FinalResultsTab(this);
    m_setupTab->setRulesets(m_rulesets);

    if (!m_rulesets.isEmpty())
    {
        EventDetails details = m_eventModel.eventDetails();
        details.rulesetId = m_rulesets.front().id;
        m_eventModel.setEventDetails(details);
    }

    m_tabWidget->addTab(m_setupTab, "Setup");
    m_tabWidget->addTab(m_finalResultsTab, "Final Results");
    m_lastActiveTab = m_setupTab;

    connect(m_setupTab, &SetupTab::eventDetailsSubmitted, this, &MainWindow::applyEventDetails);
    connect(m_setupTab, &SetupTab::importEntriesRequested, this, &MainWindow::importEntries);
    connect(m_setupTab, &SetupTab::manualCompetitorSubmitted, this, &MainWindow::addManualCompetitor);
    connect(m_setupTab, &SetupTab::competitorPreviewEdited, this, &MainWindow::updateCompetitorFromPreview);
    connect(m_setupTab, &SetupTab::deleteCompetitorsRequested, this, &MainWindow::deleteCompetitorsFromPreview);
    connect(m_setupTab, &SetupTab::autoAssignNumbersRequested, this, &MainWindow::autoAssignNumbersFromPreview);
    connect(m_setupTab, &SetupTab::printParticipantSummaryRequested, this, &MainWindow::printParticipantSummaryPdf);
    connect(m_setupTab, &SetupTab::exportCsvRequested, this, &MainWindow::exportEventCsv);
    connect(m_setupTab, &SetupTab::clearAllRequested, this, &MainWindow::clearEventData);
    connect(&m_eventModel, &EventModel::changed, this, &MainWindow::refreshViews);
    connect(&m_eventModel, &EventModel::changed, this, &MainWindow::saveState);
    connect(m_finalResultsTab, &FinalResultsTab::divisionSelectionChanged, &m_eventModel, &EventModel::setDivisionSelection);
    connect(m_finalResultsTab, &FinalResultsTab::printRequested, this, &MainWindow::printFinalResults);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::handleTabChanged);

    loadState();
    refreshDisciplineTabs();
    refreshViews();
}

void MainWindow::applyEventDetails(const EventDetails &details)
{
    m_eventModel.setEventDetails(details);
    refreshDisciplineTabs();
    refreshViews();
}

void MainWindow::importEntries(const QString &csvPath)
{
    const CsvImportResult result = m_csvImporter.importEntries(csvPath);
    m_eventModel.setCompetitors(result.competitors);

    EventDetails details = m_eventModel.eventDetails();
    details.csvPath = csvPath;
    m_eventModel.setEventDetails(details);

    const QList<DisciplineType> disciplines = details.selectedDisciplines.isEmpty()
        ? allDisciplines()
        : details.selectedDisciplines;
    for (const DisciplineType discipline : disciplines)
    {
        recalculateDiscipline(discipline);
    }

    QMap<QString, int> importedPlacings;
    const QVector<Competitor> importedCompetitors = m_eventModel.competitors();
    for (const Competitor &competitor : importedCompetitors)
    {
        const QString bib = competitor.bib.trimmed();
        if (bib.isEmpty())
        {
            continue;
        }
        if (result.bonusRoundPlacingsByBib.contains(bib))
        {
            importedPlacings.insert(bib, result.bonusRoundPlacingsByBib.value(bib));
        }
    }
    m_bonusRoundPlacingsByBib = importedPlacings;
    m_bonusRoundEnabled = result.bonusRoundEnabled || !m_bonusRoundPlacingsByBib.isEmpty();
    refreshDisciplineTabs();
    refreshViews();
    saveState();

    if (!result.warnings.isEmpty())
    {
        QMessageBox::information(this, "CSV Import", result.warnings.join("\n"));
    }
}

void MainWindow::addManualCompetitor(const Competitor &competitor)
{
    if (competitor.displayName().isEmpty())
    {
        QMessageBox::warning(this, "Manual Competitor Entry", "First or last name is required.");
        return;
    }

    m_eventModel.addCompetitor(competitor);
}

void MainWindow::updateCompetitorFromPreview(int competitorIndex, const Competitor &competitor)
{
    m_eventModel.updateCompetitor(competitorIndex, competitor);
    recalculateCompetitorScores(competitorIndex);
}

void MainWindow::deleteCompetitorsFromPreview(const QVector<int> &competitorIndexes)
{
    if (competitorIndexes.isEmpty())
    {
        return;
    }

    QVector<Competitor> competitors = m_eventModel.competitors();
    QSet<int> uniqueIndexes;
    for (const int competitorIndex : competitorIndexes)
    {
        if (competitorIndex >= 0 && competitorIndex < competitors.size())
        {
            uniqueIndexes.insert(competitorIndex);
        }
    }

    if (uniqueIndexes.isEmpty())
    {
        return;
    }

    const auto response = QMessageBox::question(
        this,
        "Delete Competitors",
        QString("Delete %1 selected competitor(s)?").arg(uniqueIndexes.size()),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (response != QMessageBox::Yes)
    {
        return;
    }

    QVector<int> sortedIndexes = uniqueIndexes.values().toVector();
    std::sort(sortedIndexes.begin(), sortedIndexes.end(), std::greater<int>());
    for (const int index : sortedIndexes)
    {
        competitors.removeAt(index);
    }

    m_eventModel.setCompetitors(competitors);
}

void MainWindow::autoAssignNumbersFromPreview(const QVector<int> &competitorIndexesInViewOrder, int startingNumber)
{
    const QString assignmentError = m_eventModel.autoAssignBibNumbers(competitorIndexesInViewOrder, startingNumber);
    if (!assignmentError.isEmpty())
    {
        QMessageBox::warning(this, "Auto Assign Numbers", assignmentError);
    }
}

void MainWindow::assignHeats(DisciplineType discipline, int lanesPerHeat)
{
    m_eventModel.assignSequentialHeatsAndLanes(discipline, lanesPerHeat);
}

void MainWindow::autoAssignFencingGroups(int numberPistes)
{
    const QString assignmentError = m_eventModel.autoAssignFencingGroups(numberPistes);
    if (!assignmentError.isEmpty())
    {
        if (m_fencingGroupTab != nullptr)
        {
            m_fencingGroupTab->setStatusMessage(assignmentError, true);
        }
        QMessageBox::warning(this, "Fencing Group Assignment", assignmentError);
        return;
    }

    int highestGroup = 0;
    int assignedCount = 0;
    for (const Competitor &competitor : m_eventModel.competitors())
    {
        highestGroup = std::max(highestGroup, competitor.fencingGroup);
        if (competitor.fencingGroup > 0)
        {
            ++assignedCount;
        }
    }

    if (m_fencingGroupTab != nullptr)
    {
        QString estimatedRuntimeSegment;
        const int estimatedMinutes = m_fencingGroupTab->estimatedRuntimeMinutes();
        const int estimatedRounds = m_fencingGroupTab->estimatedRoundCount();
        const int maxBoutsPerPistePerRound = m_fencingGroupTab->maxBoutsPerPistePerRound();
        if (estimatedRounds > 0 && maxBoutsPerPistePerRound > 0)
        {
            const int hours = estimatedMinutes / 60;
            const int minutes = estimatedMinutes % 60;
            QString durationText = QString("%1 min").arg(estimatedMinutes);
            if (hours > 0)
            {
                durationText += QString(" (%1h %2m)").arg(hours).arg(minutes, 2, 10, QChar('0'));
            }

            estimatedRuntimeSegment = QString(
                " Estimated runtime: %1 (2 min × %2 rounds × max %3 bouts/piste/round + 1 min × %2 rounds).")
                .arg(durationText)
                .arg(estimatedRounds)
                .arg(maxBoutsPerPistePerRound);
        }

        m_fencingGroupTab->setStatusMessage(
            QString("Assigned %1 fencers into %2 group(s) using %3 piste(s).%4")
                .arg(assignedCount)
                .arg(highestGroup)
                .arg(numberPistes)
                .arg(estimatedRuntimeSegment),
            false);
    }
}

void MainWindow::exportFencingGroupListPdf()
{
    const EventDetails &details = m_eventModel.eventDetails();
    QString defaultName = details.eventName.trimmed();
    if (defaultName.isEmpty())
    {
        defaultName = "fencing-group-list";
    }
    defaultName = defaultName.toLower().replace(' ', '-');
    const QString datedName = QString("%1-%2-fencing-group-list.pdf")
        .arg(defaultName, details.eventDate.isValid() ? details.eventDate.toString("yyyyMMdd") : QDate::currentDate().toString("yyyyMMdd"));

    const QString outputPath = QFileDialog::getSaveFileName(
        this,
        "Export Fencing Group List PDF",
        QDir::home().filePath(datedName),
        "PDF Files (*.pdf)");
    if (outputPath.trimmed().isEmpty())
    {
        return;
    }

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageMargins(QMarginsF(10, 12, 10, 12), QPageLayout::Millimeter);
    printer.setOutputFileName(outputPath);

    QTextDocument document;
    document.setHtml(fencingGroupListHtml());
    document.print(&printer);

    QMessageBox::information(this, "Fencing Group List PDF", QString("Fencing group list exported to:\n%1").arg(outputPath));
}

void MainWindow::exportFencingRoundSheetsPdf()
{
    const EventDetails &details = m_eventModel.eventDetails();
    QString defaultName = details.eventName.trimmed();
    if (defaultName.isEmpty())
    {
        defaultName = "fencing-round-sheets";
    }
    defaultName = defaultName.toLower().replace(' ', '-');
    const QString datedName = QString("%1-%2-fencing-round-sheets.pdf")
        .arg(defaultName, details.eventDate.isValid() ? details.eventDate.toString("yyyyMMdd") : QDate::currentDate().toString("yyyyMMdd"));

    const QString outputPath = QFileDialog::getSaveFileName(
        this,
        "Export Fencing Round Sheets PDF",
        QDir::home().filePath(datedName),
        "PDF Files (*.pdf)");
    if (outputPath.trimmed().isEmpty())
    {
        return;
    }

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(10, 12, 10, 12), QPageLayout::Millimeter);
    printer.setOutputFileName(outputPath);

    QTextDocument document;
    document.setHtml(fencingRoundSheetsHtml());
    document.print(&printer);

    QMessageBox::information(this, "Fencing Round Sheets PDF", QString("Fencing round sheets exported to:\n%1").arg(outputPath));
}

void MainWindow::exportBonusBracketSheetPdf()
{
    const EventDetails &details = m_eventModel.eventDetails();
    QString defaultName = details.eventName.trimmed();
    if (defaultName.isEmpty())
    {
        defaultName = "bonus-round-bracket";
    }
    defaultName = defaultName.toLower().replace(' ', '-');
    const QString datedName = QString("%1-%2-bonus-round-bracket.pdf")
        .arg(defaultName, details.eventDate.isValid() ? details.eventDate.toString("yyyyMMdd") : QDate::currentDate().toString("yyyyMMdd"));

    const QString outputPath = QFileDialog::getSaveFileName(
        this,
        "Export Bonus Round Bracket PDF",
        QDir::home().filePath(datedName),
        "PDF Files (*.pdf)");
    if (outputPath.trimmed().isEmpty())
    {
        return;
    }

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(10, 12, 10, 12), QPageLayout::Millimeter);
    printer.setOutputFileName(outputPath);

    QTextDocument document;
    document.setHtml(bonusBracketSheetHtml());
    document.print(&printer);

    QMessageBox::information(this, "Bonus Round Bracket PDF", QString("Bonus round bracket sheet exported to:\n%1").arg(outputPath));
}

void MainWindow::updateLaneAssignment(DisciplineType discipline, int competitorIndex, int heat, int lane)
{
    m_eventModel.updateLaneAssignment(competitorIndex, discipline, heat, lane);
}

void MainWindow::updateRawResult(DisciplineType discipline, int competitorIndex, const QString &rawResult)
{
    m_eventModel.updateRawResult(competitorIndex, discipline, rawResult);
    recalculateCompetitorScores(competitorIndex);
}

void MainWindow::updatePenalty(DisciplineType discipline, int competitorIndex, const QString &penalty)
{
    m_eventModel.updatePenalty(competitorIndex, discipline, penalty);
    recalculateCompetitorScores(competitorIndex);
}

void MainWindow::updateFencingRawResult(int competitorIndex, const QString &rawResult)
{
    m_pendingFencingRecalculation = true;
    m_eventModel.updateRawResult(competitorIndex, DisciplineType::Fencing, rawResult);
}

void MainWindow::updateFencingPenalty(int competitorIndex, const QString &penalty)
{
    m_pendingFencingRecalculation = true;
    m_eventModel.updatePenalty(competitorIndex, DisciplineType::Fencing, penalty);
}

void MainWindow::recalculateFencingResults()
{
    m_fencingPairLimit = pairLimitForFencing();
    m_pendingFencingRecalculation = false;
    recalculateFencingRankingScores();
}

void MainWindow::setBonusRoundEnabled(bool enabled)
{
    if (m_bonusRoundEnabled == enabled)
    {
        return;
    }

    m_bonusRoundEnabled = enabled;
    if (!m_bonusRoundEnabled)
    {
        m_bonusRoundPlacingsByBib.clear();
        recalculateFencingRankingScores();
    }

    refreshDisciplineTabs();
    refreshViews();
    saveState();
}

void MainWindow::updateBonusRoundPlacings(const QMap<int, int> &placementsByCompetitorIndex)
{
    QMap<QString, int> placingsByBib;
    const QVector<Competitor> competitors = m_eventModel.competitors();
    for (auto it = placementsByCompetitorIndex.begin(); it != placementsByCompetitorIndex.end(); ++it)
    {
        const int index = it.key();
        if (index < 0 || index >= competitors.size())
        {
            continue;
        }
        placingsByBib.insert(competitors.at(index).bib.trimmed(), it.value());
    }

    const bool changed = placingsByBib != m_bonusRoundPlacingsByBib;
    m_bonusRoundPlacingsByBib = placingsByBib;
    m_pendingFencingRecalculation = false;
    if (m_bonusRoundEnabled)
    {
        refreshViews();
    }
    if (changed)
    {
        saveState();
    }
}

void MainWindow::handleTabChanged(int index)
{
    QWidget *newTab = m_tabWidget->widget(index);
    const bool leavingFencingWorkflow = (m_lastActiveTab == m_fencingResultsTab || m_lastActiveTab == m_bonusRoundTab)
        && newTab != m_fencingResultsTab
        && newTab != m_bonusRoundTab;

    if (leavingFencingWorkflow)
    {
        if (m_bonusRoundTab != nullptr)
        {
            m_bonusRoundTab->applyPlacings();
        }
        if (m_pendingFencingRecalculation)
        {
            recalculateFencingResults();
        }
    }

    m_lastActiveTab = newTab;
}

int MainWindow::pairLimitForFencing() const
{
    if (m_fencingResultsTab != nullptr)
    {
        return std::clamp(m_fencingResultsTab->pairLimit(), 1, 10);
    }
    return std::clamp(m_fencingPairLimit, 1, 10);
}

QVector<Competitor> MainWindow::competitorsForFinalResults() const
{
    QVector<Competitor> competitors = m_eventModel.competitors();
    if (!m_bonusRoundEnabled)
    {
        return competitors;
    }

    for (int index = 0; index < competitors.size(); ++index)
    {
        Competitor &competitor = competitors[index];
        competitor.scores.insert(DisciplineType::Fencing, computedFencingBonusScore(competitor));
    }
    return competitors;
}

QVector<Competitor> MainWindow::rankedCompetitorsForFinalResults(const DivisionSelection &selection) const
{
    auto categorySortKey = [](const QString &category) {
        const QString normalized = category.trimmed().toLower();
        if (normalized.isEmpty())
        {
            return QString("9-999-unspecified");
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
    };

    auto compareDivision = [this, &categorySortKey](const Competitor &left, const Competitor &right, ResultDivision division) {
        if (division == ResultDivision::AgeCategory)
        {
            const QString leftCategoryKey = categorySortKey(left.category);
            const QString rightCategoryKey = categorySortKey(right.category);
            if (leftCategoryKey != rightCategoryKey)
            {
                return leftCategoryKey < rightCategoryKey;
            }
        }

        const QString leftKey = m_eventModel.divisionKeyForCompetitor(left, division).toLower();
        const QString rightKey = m_eventModel.divisionKeyForCompetitor(right, division).toLower();
        if (leftKey != rightKey)
        {
            return leftKey < rightKey;
        }

        return false;
    };

    QVector<Competitor> ranked = competitorsForFinalResults();
    std::sort(ranked.begin(), ranked.end(), [this, &selection, &compareDivision](const Competitor &left, const Competitor &right) {
        if (compareDivision(left, right, selection.primary))
        {
            return true;
        }
        if (compareDivision(right, left, selection.primary))
        {
            return false;
        }

        if (compareDivision(left, right, selection.secondary))
        {
            return true;
        }
        if (compareDivision(right, left, selection.secondary))
        {
            return false;
        }

        const int leftTotal = m_eventModel.competitorTotalScore(left);
        const int rightTotal = m_eventModel.competitorTotalScore(right);
        if (leftTotal == rightTotal)
        {
            return left.displayName().toLower() < right.displayName().toLower();
        }
        return leftTotal > rightTotal;
    });
    return ranked;
}

int MainWindow::computedFencingRankingScore(const Competitor &competitor) const
{
    if (!competitorParticipatesInDiscipline(competitor, DisciplineType::Fencing))
    {
        return 0;
    }

    if (competitorHasDsq(competitor))
    {
        return 0;
    }

    const QString penalty = competitor.penalties.value(DisciplineType::Fencing).trimmed();
    if (isElmPenalty(penalty))
    {
        return 0;
    }

    const QVector<Competitor> competitors = m_eventModel.competitors();
    QVector<const Competitor *> fencingCompetitors;
    fencingCompetitors.reserve(competitors.size());
    for (const Competitor &candidate : competitors)
    {
        if (competitorParticipatesInDiscipline(candidate, DisciplineType::Fencing))
        {
            fencingCompetitors.push_back(&candidate);
        }
    }

    const int pairLimit = pairLimitForFencing();
    const int boutsInCompetition = std::max(0, static_cast<int>(fencingCompetitors.size()) - 1) * pairLimit;
    const QString selfBib = competitor.bib.trimmed();
    const QMap<QString, int> selfRow = parseFencingMatrixRowRaw(competitor.rawResults.value(DisciplineType::Fencing));

    int victories = 0;
    for (const Competitor *opponent : fencingCompetitors)
    {
        if (opponent == nullptr || opponent->bib.trimmed() == selfBib)
        {
            continue;
        }

        victories += std::max(0, selfRow.value(opponent->bib.trimmed(), 0));
    }

    const int baseScore = fencingLinearScoreFromVictories(victories, boutsInCompetition);
    const auto penaltyValue = integerPenaltyValue(penalty);
    if (penaltyValue.has_value())
    {
        return std::max(0, baseScore - *penaltyValue);
    }
    return baseScore;
}

int MainWindow::computedFencingBonusScore(const Competitor &competitor) const
{
    if (!competitorParticipatesInDiscipline(competitor, DisciplineType::Fencing))
    {
        return 0;
    }

    if (competitorHasDsq(competitor))
    {
        return 0;
    }

    const QString penalty = competitor.penalties.value(DisciplineType::Fencing).trimmed();
    if (isElmPenalty(penalty))
    {
        return 0;
    }

    static const QMap<int, int> scoreByRank = loadBonusRoundScoringTable();
    const int rank = m_bonusRoundPlacingsByBib.value(competitor.bib.trimmed(), 0);
    int baseScore = scoreByRank.value(rank, 0);
    const auto penaltyValue = integerPenaltyValue(penalty);
    if (penaltyValue.has_value())
    {
        baseScore = std::max(0, baseScore - *penaltyValue);
    }
    return baseScore;
}

void MainWindow::recalculateFencingRankingScores()
{
    QVector<Competitor> updatedCompetitors = m_eventModel.competitors();
    for (int index = 0; index < updatedCompetitors.size(); ++index)
    {
        updatedCompetitors[index].scores.insert(
            DisciplineType::Fencing,
            computedFencingRankingScore(updatedCompetitors.at(index)));
    }
    m_eventModel.setCompetitors(updatedCompetitors);
}

int MainWindow::computedScoreForCompetitor(const Competitor &competitor, DisciplineType discipline, const DisciplineRule *rule) const
{
    if (!competitorParticipatesInDiscipline(competitor, discipline))
    {
        return 0;
    }

    if (competitorHasDsq(competitor))
    {
        return 0;
    }

    const QString penalty = competitor.penalties.value(discipline).trimmed();
    if (isElmPenalty(penalty))
    {
        return 0;
    }

    if (discipline == DisciplineType::Fencing)
    {
        return computedFencingRankingScore(competitor);
    }

    int baseScore = 0;
    const QString scoringTime = scoringTimeFromRaw(discipline, competitor.rawResults.value(discipline));
    if (!isNoTimeRecorded(rule, scoringTime) && rule != nullptr)
    {
        DisciplineRule scoringRule = *rule;
        if (discipline == DisciplineType::LaserRun)
        {
            scoringRule.referenceMilliseconds = laserRunReferenceMillisecondsForCategory(competitor.category);
        }
        else if (discipline == DisciplineType::Swimming)
        {
            scoringRule.referenceMilliseconds = swimmingReferenceMillisecondsForCategory(competitor.category, competitor.gender);
            scoringRule.millisecondsPerPoint = swimmingMillisecondsPerPointForCategory(competitor.category);
            scoringRule.referencePoints = 250;
            scoringRule.timeRounding = swimmingTimeRoundingForCategory(competitor.category);
        }

        const auto score = scoreRawResult(scoringRule, scoringTime);
        if (score.has_value())
        {
            baseScore = *score;
        }
    }

    const auto penaltyValue = integerPenaltyValue(penalty);
    if (penaltyValue.has_value())
    {
        return baseScore - *penaltyValue;
    }
    return baseScore;
}

void MainWindow::recalculateCompetitorScores(int competitorIndex)
{
    const Ruleset *ruleset = currentRuleset();
    if (ruleset == nullptr)
    {
        return;
    }

    const QVector<Competitor> competitors = m_eventModel.competitors();
    if (competitorIndex < 0 || competitorIndex >= competitors.size())
    {
        return;
    }

    const QList<DisciplineType> disciplines = m_eventModel.eventDetails().selectedDisciplines.isEmpty()
        ? allDisciplines()
        : m_eventModel.eventDetails().selectedDisciplines;

    const Competitor &competitor = competitors.at(competitorIndex);
    for (const DisciplineType selectedDiscipline : disciplines)
    {
        const DisciplineRule *selectedRule = ruleset->ruleFor(selectedDiscipline);
        m_eventModel.updateScore(competitorIndex, selectedDiscipline, computedScoreForCompetitor(competitor, selectedDiscipline, selectedRule));
    }
}

bool MainWindow::competitorHasDsq(const Competitor &competitor) const
{
    const QList<DisciplineType> disciplines = m_eventModel.eventDetails().selectedDisciplines.isEmpty()
        ? allDisciplines()
        : m_eventModel.eventDetails().selectedDisciplines;
    for (const DisciplineType discipline : disciplines)
    {
        if (isDsqPenalty(competitor.penalties.value(discipline)))
        {
            return true;
        }
    }
    return false;
}

void MainWindow::recalculateDiscipline(DisciplineType discipline)
{
    const Ruleset *ruleset = currentRuleset();
    if (ruleset == nullptr)
    {
        return;
    }

    const DisciplineRule *rule = ruleset->ruleFor(discipline);
    const QVector<Competitor> competitors = m_eventModel.competitors();
    for (int index = 0; index < competitors.size(); ++index)
    {
        m_eventModel.updateScore(index, discipline, computedScoreForCompetitor(competitors.at(index), discipline, rule));
    }
}

void MainWindow::printBlankSheet(DisciplineType discipline)
{
    printHtmlDocument(QString("%1 Blank Sheet").arg(disciplineDisplayName(discipline)), disciplineTableHtml(discipline, false));
}

void MainWindow::printDisciplineResults(DisciplineType discipline)
{
    printHtmlDocument(QString("%1 Times").arg(disciplineDisplayName(discipline)), disciplineTableHtml(discipline, true));
}

void MainWindow::printFinalResults()
{
    printHtmlDocument("Final Results", finalResultsHtml());
}

void MainWindow::printParticipantSummaryPdf()
{
    const EventDetails &details = m_eventModel.eventDetails();
    QString defaultName = details.eventName.trimmed();
    if (defaultName.isEmpty())
    {
        defaultName = "participant-summary";
    }
    defaultName = defaultName.toLower().replace(' ', '-');
    const QString datedName = QString("%1-%2-participant-summary.pdf")
        .arg(defaultName, details.eventDate.isValid() ? details.eventDate.toString("yyyyMMdd") : QDate::currentDate().toString("yyyyMMdd"));

    const QString outputPath = QFileDialog::getSaveFileName(
        this,
        "Export Participant Summary PDF",
        QDir::home().filePath(datedName),
        "PDF Files (*.pdf)");
    if (outputPath.trimmed().isEmpty())
    {
        return;
    }

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(10, 12, 10, 12), QPageLayout::Millimeter);
    printer.setOutputFileName(outputPath);

    QTextDocument document;
    document.setHtml(participantSummaryHtml());
    document.print(&printer);

    QMessageBox::information(this, "Participant Summary PDF", QString("Participant summary exported to:\n%1").arg(outputPath));
}

void MainWindow::exportEventCsv()
{
    const EventDetails &details = m_eventModel.eventDetails();
    QString defaultName = details.eventName.trimmed();
    if (defaultName.isEmpty())
    {
        defaultName = "event-data";
    }
    defaultName = defaultName.toLower().replace(' ', '-');
    const QString datedName = QString("%1-%2.csv")
        .arg(defaultName, details.eventDate.isValid() ? details.eventDate.toString("yyyyMMdd") : QDate::currentDate().toString("yyyyMMdd"));

    const QString outputPath = QFileDialog::getSaveFileName(
        this,
        "Export Event CSV",
        QDir::home().filePath(datedName),
        "CSV Files (*.csv)");
    if (outputPath.trimmed().isEmpty())
    {
        return;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Export Event CSV", QString("Could not write to %1").arg(outputPath));
        return;
    }

    QTextStream stream(&file);

    QStringList headers = {
        "event_name", "event_date", "ruleset", "number", "first_name", "last_name", "gender", "birth_year", "category", "category_override", "affiliation",
        "fence_enabled", "obstacle_enabled", "swim_enabled", "laser_run_enabled",
        "bonus_round_enabled", "bonus_round_rank", "bonus_round_score"
    };
    const QList<DisciplineType> disciplines = allDisciplines();
    for (const DisciplineType discipline : disciplines)
    {
        const QString key = disciplineKey(discipline);
        headers << QString("%1_heat").arg(key)
                << QString("%1_lane").arg(key)
                << QString("%1_time").arg(key)
                << QString("%1_penalty").arg(key)
                << QString("%1_score").arg(key);
    }
    headers << "total_score";
    stream << headers.join(',') << "\n";

    const QVector<Competitor> exportCompetitors = competitorsForFinalResults();
    for (const Competitor &competitor : exportCompetitors)
    {
        const QString bib = competitor.bib.trimmed();
        const int bonusRank = m_bonusRoundPlacingsByBib.value(bib, 0);
        const int bonusScore = bonusRank > 0 ? computedFencingBonusScore(competitor) : 0;

        QStringList row = {
            csvEscape(details.eventName),
            csvEscape(details.eventDate.isValid() ? details.eventDate.toString(Qt::ISODate) : QString()),
            csvEscape(details.rulesetId),
            csvEscape(competitor.bib),
            csvEscape(competitor.firstName),
            csvEscape(competitor.lastName),
            csvEscape(competitor.gender),
            csvEscape(competitor.birthYear > 0 ? QString::number(competitor.birthYear) : QString()),
            csvEscape(competitor.category),
            csvEscape(competitor.categoryOverride ? "1" : "0"),
            csvEscape(competitor.affiliation),
            csvEscape(competitorParticipatesInDiscipline(competitor, DisciplineType::Fencing) ? "1" : "0"),
            csvEscape(competitorParticipatesInDiscipline(competitor, DisciplineType::Obstacle) ? "1" : "0"),
            csvEscape(competitorParticipatesInDiscipline(competitor, DisciplineType::Swimming) ? "1" : "0"),
            csvEscape(competitorParticipatesInDiscipline(competitor, DisciplineType::LaserRun) ? "1" : "0"),
            csvEscape(m_bonusRoundEnabled ? "1" : "0"),
            csvEscape(bonusRank > 0 ? QString::number(bonusRank) : QString()),
            csvEscape(bonusRank > 0 ? QString::number(bonusScore) : QString())
        };

        for (const DisciplineType discipline : disciplines)
        {
            const LaneAssignment assignment = competitor.laneAssignments.value(discipline);
            row << csvEscape(assignment.heat > 0 ? QString::number(assignment.heat) : QString())
                << csvEscape(assignment.lane > 0 ? QString::number(assignment.lane) : QString())
                << csvEscape(scoringTimeFromRaw(discipline, competitor.rawResults.value(discipline)))
                << csvEscape(competitor.penalties.value(discipline))
                << csvEscape(disciplineScoreDisplayValue(competitor, discipline));
        }
        row << csvEscape(QString::number(m_eventModel.competitorTotalScore(competitor)));
        stream << row.join(',') << "\n";
    }

    QMessageBox::information(this, "Export Event CSV", QString("Event data exported to:\n%1").arg(outputPath));
}

void MainWindow::clearEventData()
{
    const auto result = QMessageBox::warning(
        this,
        "Clear Event Data",
        "This will permanently clear current event setup, entries, heats, times, penalties, and scores.\n\nPlease export a CSV backup first.\n\nDo you want to continue?",
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (result != QMessageBox::Yes)
    {
        return;
    }

    EventDetails details;
    details.eventDate = QDate::currentDate();
    details.selectedDisciplines = allDisciplines();
    details.rulesetId = m_eventModel.eventDetails().rulesetId;
    if (details.rulesetId.isEmpty() && !m_rulesets.isEmpty())
    {
        details.rulesetId = m_rulesets.front().id;
    }

    m_eventModel.setCompetitors({});
    m_eventModel.setDivisionSelection(DivisionSelection{});
    m_eventModel.setEventDetails(details);
    m_bonusRoundEnabled = false;
    m_bonusRoundPlacingsByBib.clear();
    refreshDisciplineTabs();
    refreshViews();

    QMessageBox::information(this, "Clear Event Data", "Event data has been cleared.");
}

void MainWindow::refreshViews()
{
    m_setupTab->setEventDetails(m_eventModel.eventDetails());
    m_setupTab->setCompetitorPreview(m_eventModel.competitors());
    m_finalResultsTab->setDivisionSelection(m_eventModel.divisionSelection());
    m_finalResultsTab->populate(
        rankedCompetitorsForFinalResults(m_eventModel.divisionSelection()),
        m_eventModel,
        m_eventModel.eventDetails().selectedDisciplines);

    const QWidget *activeTab = m_tabWidget != nullptr ? m_tabWidget->currentWidget() : nullptr;
    const bool deferFencingPopulate = m_pendingFencingRecalculation
        && (activeTab == m_fencingResultsTab || activeTab == m_bonusRoundTab);

    if (m_fencingGroupTab != nullptr)
    {
        m_fencingGroupTab->populate(m_eventModel.competitors());
    }

    if (!deferFencingPopulate && m_fencingResultsTab != nullptr && m_fencingGroupTab != nullptr)
    {
        m_fencingResultsTab->setBonusRoundEnabled(m_bonusRoundEnabled);
        m_fencingResultsTab->setPairLimit(m_fencingPairLimit);
        m_fencingResultsTab->setRoundSchedule(m_fencingGroupTab->roundScheduleRows());
        m_fencingResultsTab->populate(m_eventModel.competitors());
    }

    if (!deferFencingPopulate && m_bonusRoundTab != nullptr && m_fencingResultsTab != nullptr)
    {
        m_bonusRoundTab->populate(m_eventModel.competitors(), m_fencingResultsTab->rankedCompetitorIndexes());
        m_bonusRoundTab->setPlacingsByBib(m_bonusRoundPlacingsByBib);
    }

    const Ruleset *ruleset = currentRuleset();
    for (auto it = m_disciplineTabs.begin(); it != m_disciplineTabs.end(); ++it)
    {
        it.value()->setRuleSummary(ruleset != nullptr ? ruleset->ruleFor(it.key()) : nullptr);
        it.value()->populate(m_eventModel.competitors());
    }
}

const Ruleset *MainWindow::currentRuleset() const
{
    const QString rulesetId = m_eventModel.eventDetails().rulesetId;
    for (const Ruleset &ruleset : m_rulesets)
    {
        if (ruleset.id == rulesetId)
        {
            return &ruleset;
        }
    }

    return m_rulesets.isEmpty() ? nullptr : &m_rulesets.front();
}

void MainWindow::refreshDisciplineTabs()
{
    if (m_fencingGroupTab != nullptr)
    {
        const int index = m_tabWidget->indexOf(m_fencingGroupTab);
        if (index >= 0)
        {
            m_tabWidget->removeTab(index);
        }
        m_fencingGroupTab->deleteLater();
        m_fencingGroupTab = nullptr;
    }
    if (m_fencingResultsTab != nullptr)
    {
        const int index = m_tabWidget->indexOf(m_fencingResultsTab);
        if (index >= 0)
        {
            m_tabWidget->removeTab(index);
        }
        m_fencingResultsTab->deleteLater();
        m_fencingResultsTab = nullptr;
    }
    if (m_bonusRoundTab != nullptr)
    {
        const int index = m_tabWidget->indexOf(m_bonusRoundTab);
        if (index >= 0)
        {
            m_tabWidget->removeTab(index);
        }
        m_bonusRoundTab->deleteLater();
        m_bonusRoundTab = nullptr;
    }

    for (auto it = m_disciplineTabs.begin(); it != m_disciplineTabs.end(); ++it)
    {
        const int index = m_tabWidget->indexOf(it.value());
        if (index >= 0)
        {
            m_tabWidget->removeTab(index);
        }
        it.value()->deleteLater();
    }
    m_disciplineTabs.clear();

    const QList<DisciplineType> disciplines = m_eventModel.eventDetails().selectedDisciplines.isEmpty()
        ? allDisciplines()
        : m_eventModel.eventDetails().selectedDisciplines;

    int insertIndex = 1;
    if (disciplines.contains(DisciplineType::Fencing))
    {
        m_fencingGroupTab = new FencingGroupTab(this);
        connect(m_fencingGroupTab, &FencingGroupTab::autoAssignGroupsRequested, this, &MainWindow::autoAssignFencingGroups);
        connect(m_fencingGroupTab, &FencingGroupTab::exportGroupListRequested, this, &MainWindow::exportFencingGroupListPdf);
        connect(m_fencingGroupTab, &FencingGroupTab::exportRoundSheetsRequested, this, &MainWindow::exportFencingRoundSheetsPdf);
        m_tabWidget->insertTab(insertIndex++, m_fencingGroupTab, "Fencing Groups");

        m_fencingResultsTab = new FencingResultsTab(this);
        connect(m_fencingResultsTab, &FencingResultsTab::rawResultChanged, this, &MainWindow::updateFencingRawResult);
        connect(m_fencingResultsTab, &FencingResultsTab::penaltyChanged, this, &MainWindow::updateFencingPenalty);
        connect(m_fencingResultsTab, &FencingResultsTab::recalculateRequested, this, &MainWindow::recalculateFencingResults, Qt::QueuedConnection);
        connect(m_fencingResultsTab, &FencingResultsTab::bonusRoundEnabledChanged, this, &MainWindow::setBonusRoundEnabled);
        m_tabWidget->insertTab(insertIndex++, m_fencingResultsTab, "Fencing Results");

        if (m_bonusRoundEnabled)
        {
            m_bonusRoundTab = new BonusRoundTab(this);
            connect(m_bonusRoundTab, &BonusRoundTab::standingsChanged, this, &MainWindow::updateBonusRoundPlacings);
            connect(m_bonusRoundTab, &BonusRoundTab::exportBracketSheetRequested, this, &MainWindow::exportBonusBracketSheetPdf);
            m_tabWidget->insertTab(insertIndex++, m_bonusRoundTab, "Bonus Round");
        }
    }

    for (const DisciplineType discipline : disciplines)
    {
        if (discipline == DisciplineType::Fencing)
        {
            continue;
        }
        auto *tab = new DisciplineTab(discipline, this);
        connect(tab, &DisciplineTab::assignHeatsRequested, this, &MainWindow::assignHeats);
        connect(tab, &DisciplineTab::laneAssignmentChanged, this, &MainWindow::updateLaneAssignment);
        connect(tab, &DisciplineTab::rawResultChanged, this, &MainWindow::updateRawResult);
        connect(tab, &DisciplineTab::penaltyChanged, this, &MainWindow::updatePenalty);
        connect(tab, &DisciplineTab::recalculateRequested, this, &MainWindow::recalculateDiscipline);
        connect(tab, &DisciplineTab::printBlankRequested, this, &MainWindow::printBlankSheet);
        connect(tab, &DisciplineTab::printResultsRequested, this, &MainWindow::printDisciplineResults);
        m_tabWidget->insertTab(insertIndex++, tab, disciplineDisplayName(discipline));
        m_disciplineTabs.insert(discipline, tab);
    }

    m_lastActiveTab = m_tabWidget->currentWidget();
}

QString MainWindow::disciplineTableHtml(DisciplineType discipline, bool includeResults) const
{
    QString html;
    html += reportHeaderHtml(includeResults
        ? QString("%1 Results").arg(disciplineDisplayName(discipline))
        : disciplineDisplayName(discipline));

    QMap<int, QVector<const Competitor *>> groupedByHeat;
    QVector<const Competitor *> unassigned;
    for (const Competitor &competitor : m_eventModel.competitors())
    {
        if (!competitorParticipatesInDiscipline(competitor, discipline))
        {
            continue;
        }

        const LaneAssignment assignment = competitor.laneAssignments.value(discipline);
        if (assignment.heat <= 0)
        {
            unassigned.push_back(&competitor);
        }
        else
        {
            groupedByHeat[assignment.heat].push_back(&competitor);
        }
    }

    bool firstHeatSection = true;
    for (auto it = groupedByHeat.begin(); it != groupedByHeat.end(); ++it)
    {
        if (includeResults && !firstHeatSection)
        {
            html += "<div style='page-break-before: always;'></div>";
        }
        QVector<const Competitor *> competitors = it.value();
        std::sort(competitors.begin(), competitors.end(), [discipline](const Competitor *left, const Competitor *right) {
            const int leftLane = left->laneAssignments.value(discipline).lane;
            const int rightLane = right->laneAssignments.value(discipline).lane;
            if (leftLane == rightLane)
            {
                return left->displayName().toLower() < right->displayName().toLower();
            }
            if (leftLane <= 0)
            {
                return false;
            }
            if (rightLane <= 0)
            {
                return true;
            }
            return leftLane < rightLane;
        });

        html += QString("<h2>%1</h2>").arg(includeResults ? QString("Results - Heat %1").arg(it.key()) : QString("Heat %1").arg(it.key()));
        html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
        if (discipline == DisciplineType::LaserRun)
        {
            int maxLapCount = 2;
            for (const Competitor *competitor : competitors)
            {
                maxLapCount = std::max(maxLapCount, laserLapCountForCategory(competitor->category));
            }

            html += "<tr><th>Lane</th><th>Number</th><th>Name</th><th>Gender</th><th>Category</th>";
            for (int lap = 1; lap <= maxLapCount; ++lap)
            {
                html += QString("<th>Lap %1</th>").arg(lap);
            }
            html += "<th>Penalty</th><th>Score</th></tr>";

            for (const Competitor *competitor : competitors)
            {
                const LaneAssignment assignment = competitor->laneAssignments.value(discipline);
                const int lapCount = laserLapCountForCategory(competitor->category);
                html += "<tr>";
                html += QString("<td>%1</td>").arg(assignment.lane > 0 ? QString::number(assignment.lane) : "&nbsp;");
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->bib));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->displayName()));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->gender));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->category));
                for (int lap = 1; lap <= maxLapCount; ++lap)
                {
                    const bool isScoringLap = (lap == lapCount);
                    const QString lapValue = (includeResults && isScoringLap)
                        ? scoringTimeFromRaw(DisciplineType::LaserRun, competitor->rawResults.value(discipline))
                        : QString();
                    html += QString("<td>%1</td>").arg(lapValue.isEmpty() ? "&nbsp;" : htmlEscape(lapValue));
                }
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(competitor->penalties.value(discipline)) : "&nbsp;");
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(disciplineScoreDisplayValue(*competitor, discipline)) : "&nbsp;");
                html += "</tr>";
            }
        }
        else
        {
            html += "<tr><th>Lane</th><th>Number</th><th>Name</th><th>Gender</th><th>Category</th><th>Time</th><th>Penalty</th><th>Score</th></tr>";
            for (const Competitor *competitor : competitors)
            {
                const LaneAssignment assignment = competitor->laneAssignments.value(discipline);
                html += "<tr>";
                html += QString("<td>%1</td>").arg(assignment.lane > 0 ? QString::number(assignment.lane) : "&nbsp;");
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->bib));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->displayName()));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->gender));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->category));
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(scoringTimeFromRaw(discipline, competitor->rawResults.value(discipline))) : "&nbsp;");
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(competitor->penalties.value(discipline)) : "&nbsp;");
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(disciplineScoreDisplayValue(*competitor, discipline)) : "&nbsp;");
                html += "</tr>";
            }
        }

        html += "</table>";
        firstHeatSection = false;
    }

    if (!unassigned.isEmpty())
    {
        if (includeResults && !firstHeatSection)
        {
            html += "<div style='page-break-before: always;'></div>";
        }
        std::sort(unassigned.begin(), unassigned.end(), [](const Competitor *left, const Competitor *right) {
            return left->displayName().toLower() < right->displayName().toLower();
        });

        html += QString("<h2>%1</h2>").arg(includeResults ? "Results - Unassigned Heat" : "Unassigned Heat");
        html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
        if (discipline == DisciplineType::LaserRun)
        {
            int maxLapCount = 2;
            for (const Competitor *competitor : unassigned)
            {
                maxLapCount = std::max(maxLapCount, laserLapCountForCategory(competitor->category));
            }

            html += "<tr><th>Number</th><th>Name</th><th>Gender</th><th>Category</th>";
            for (int lap = 1; lap <= maxLapCount; ++lap)
            {
                html += QString("<th>Lap %1</th>").arg(lap);
            }
            html += "<th>Penalty</th><th>Score</th></tr>";

            for (const Competitor *competitor : unassigned)
            {
                const int lapCount = laserLapCountForCategory(competitor->category);
                html += "<tr>";
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->bib));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->displayName()));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->gender));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->category));
                for (int lap = 1; lap <= maxLapCount; ++lap)
                {
                    const bool isScoringLap = (lap == lapCount);
                    const QString lapValue = (includeResults && isScoringLap)
                        ? scoringTimeFromRaw(DisciplineType::LaserRun, competitor->rawResults.value(discipline))
                        : QString();
                    html += QString("<td>%1</td>").arg(lapValue.isEmpty() ? "&nbsp;" : htmlEscape(lapValue));
                }
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(competitor->penalties.value(discipline)) : "&nbsp;");
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(disciplineScoreDisplayValue(*competitor, discipline)) : "&nbsp;");
                html += "</tr>";
            }
        }
        else
        {
            html += "<tr><th>Number</th><th>Name</th><th>Gender</th><th>Category</th><th>Time</th><th>Penalty</th><th>Score</th></tr>";
            for (const Competitor *competitor : unassigned)
            {
                html += "<tr>";
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->bib));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->displayName()));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->gender));
                html += QString("<td>%1</td>").arg(htmlEscape(competitor->category));
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(scoringTimeFromRaw(discipline, competitor->rawResults.value(discipline))) : "&nbsp;");
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(competitor->penalties.value(discipline)) : "&nbsp;");
                html += QString("<td>%1</td>").arg(includeResults ? htmlEscape(disciplineScoreDisplayValue(*competitor, discipline)) : "&nbsp;");
                html += "</tr>";
            }
        }

        html += "</table>";
    }

    return html;
}

QString MainWindow::participantSummaryHtml() const
{
    QString html = reportHeaderHtml("Participant Summary");
    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
    html += "<tr><th>Number</th><th>Name</th><th>Gender</th><th>Birth Year</th><th>Category</th><th>Affiliation</th><th>Fence</th><th>Obstacle</th><th>Swim</th><th>Laser Run</th></tr>";

    auto participationCellHtml = [](bool enabled) {
        return enabled
            ? QString("<td style='background-color:#d4edda; color:#155724; text-align:center; font-weight:600;'>Yes</td>")
            : QString("<td style='background-color:#f8d7da; color:#721c24; text-align:center; font-weight:600;'>No</td>");
    };

    for (const Competitor &competitor : m_eventModel.competitors())
    {
        html += "<tr>";
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.bib));
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.displayName()));
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.gender));
        html += QString("<td>%1</td>").arg(competitor.birthYear > 0 ? QString::number(competitor.birthYear) : "&nbsp;");
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.category));
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.affiliation));
        html += participationCellHtml(competitorParticipatesInDiscipline(competitor, DisciplineType::Fencing));
        html += participationCellHtml(competitorParticipatesInDiscipline(competitor, DisciplineType::Obstacle));
        html += participationCellHtml(competitorParticipatesInDiscipline(competitor, DisciplineType::Swimming));
        html += participationCellHtml(competitorParticipatesInDiscipline(competitor, DisciplineType::LaserRun));
        html += "</tr>";
    }

    html += "</table>";
    return html;
}

QString MainWindow::fencingGroupListHtml() const
{
    QString html = reportHeaderHtml("Fencing Group List");

    QMap<int, QVector<const Competitor *>> groups;
    QVector<const Competitor *> unassigned;
    for (const Competitor &competitor : m_eventModel.competitors())
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

    auto groupLabelForIndex = [](int groupIndex) {
        if (groupIndex <= 0)
        {
            return QString("?");
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
    };

    constexpr int maxSectionsPerPage = 4;
    
    int sectionsOnCurrentPage = 0;
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

        if (sectionsOnCurrentPage >= maxSectionsPerPage)
        {
            html += "<div style='page-break-before: always;'></div>";
            html += reportHeaderHtml("Fencing Group List");
            sectionsOnCurrentPage = 0;
        }

        html += QString("<h2>Group %1</h2>").arg(htmlEscape(groupLabelForIndex(it.key())));
        html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
        html += "<tr><th>Number</th><th>Name</th><th>Gender</th><th>Category</th><th>Affiliation</th></tr>";
        for (const Competitor *competitor : sorted)
        {
            html += "<tr>";
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->bib));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->displayName()));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->gender));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->category));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->affiliation));
            html += "</tr>";
        }
        html += "</table>";
        ++sectionsOnCurrentPage;
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

        if (sectionsOnCurrentPage >= maxSectionsPerPage)
        {
            html += "<div style='page-break-before: always;'></div>";
            html += reportHeaderHtml("Fencing Group List");
            sectionsOnCurrentPage = 0;
        }

        html += "<h2>Unassigned</h2>";
        html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
        html += "<tr><th>Number</th><th>Name</th><th>Gender</th><th>Category</th><th>Affiliation</th></tr>";
        for (const Competitor *competitor : unassigned)
        {
            html += "<tr>";
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->bib));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->displayName()));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->gender));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->category));
            html += QString("<td>%1</td>").arg(htmlEscape(competitor->affiliation));
            html += "</tr>";
        }
        html += "</table>";
    }

    return html;
}

QString MainWindow::fencingRoundSheetsHtml() const
{
    QString html;
    if (m_fencingGroupTab == nullptr)
    {
        html += reportHeaderHtml("Fencing Round Sheets");
        html += "<p>Fencing groups are not available for this event.</p>";
        return html;
    }

    const QVector<FencingRoundScheduleRow> scheduleRows = m_fencingGroupTab->roundScheduleRows();
    if (scheduleRows.isEmpty())
    {
        html += reportHeaderHtml("Fencing Round Sheets");
        html += "<p>No round schedule is available. Please assign fencing groups first.</p>";
        return html;
    }

    QMap<int, QVector<const Competitor *>> groups;
    for (const Competitor &competitor : m_eventModel.competitors())
    {
        if (!competitorParticipatesInDiscipline(competitor, DisciplineType::Fencing) || competitor.fencingGroup <= 0)
        {
            continue;
        }
        groups[competitor.fencingGroup].push_back(&competitor);
    }

    auto groupLabelForIndex = [](int groupIndex) {
        if (groupIndex <= 0)
        {
            return QString("?");
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
    };

    QMap<QString, QString> fencerNameByDescriptor;
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

        const QString groupLabel = groupLabelForIndex(it.key());
        for (int index = 0; index < sorted.size(); ++index)
        {
            const QString descriptor = QString("%1%2").arg(groupLabel).arg(index + 1);
            fencerNameByDescriptor.insert(
                descriptor,
                QString("%1 (%2)").arg(sorted.at(index)->displayName(), sorted.at(index)->bib));
        }
    }

    QStringList pageKeysInOrder;
    QMap<QString, QVector<FencingRoundScheduleRow>> scheduleRowsByPage;
    for (const FencingRoundScheduleRow &scheduleRow : scheduleRows)
    {
        const QString pageKey = QString("%1|%2").arg(scheduleRow.round).arg(scheduleRow.piste);
        if (!scheduleRowsByPage.contains(pageKey))
        {
            pageKeysInOrder.push_back(pageKey);
        }
        scheduleRowsByPage[pageKey].push_back(scheduleRow);
    }

    int pageIndex = 0;
    for (const QString &pageKey : pageKeysInOrder)
    {
        const QVector<FencingRoundScheduleRow> pageRows = scheduleRowsByPage.value(pageKey);
        if (pageRows.isEmpty())
        {
            continue;
        }

        const int roundNumber = pageRows.first().round;
        const int pisteNumber = pageRows.first().piste;
        QStringList matchupLabels;
        for (const FencingRoundScheduleRow &pageRow : pageRows)
        {
            if (!matchupLabels.contains(pageRow.matchup))
            {
                matchupLabels.push_back(pageRow.matchup);
            }
        }

        if (pageIndex > 0)
        {
            html += "<div style='page-break-before: always;'></div>";
        }

        html += reportHeaderHtml("Fencing Round Sheet");
        html += "<table border='1' cellspacing='0' cellpadding='4' width='100%' style='margin-bottom:10px;'>";
        html += "<tr>";
        html += QString("<td width='33%%'><strong>Round #</strong><br>%1</td>")
            .arg(roundNumber == 0 ? "0 (Intra Group)" : QString::number(roundNumber));
        html += QString("<td width='34%%'><strong>Groups Fencing</strong><br>%1</td>")
            .arg(htmlEscape(matchupLabels.join(", ")));
        html += QString("<td width='33%%'><strong>Piste Name</strong><br>Piste %1</td>")
            .arg(pisteNumber);
        html += "</tr>";
        html += "</table>";

        html += "<table border='1' cellspacing='0' cellpadding='4' width='100%' style='margin-bottom:12px;'>";
        html += "<tr><th width='8%'>#</th><th>Name</th><th width='10%'>Score</th><th width='10%'>Score</th><th>Name</th></tr>";
        QVector<QString> fencerDescriptorsInOrder;
        
        int boutNumber = 1;
        for (const FencingRoundScheduleRow &pageRow : pageRows)
        {
            const QStringList boutEntries = pageRow.bouts.split(',', Qt::SkipEmptyParts);
            for (const QString &entry : boutEntries)
            {
                const QString trimmed = entry.trimmed();
                if (trimmed.isEmpty())
                {
                    continue;
                }

                const QStringList pair = trimmed.split('-', Qt::KeepEmptyParts);
                QString leftDescriptor = trimmed;
                QString rightDescriptor = QString();
                if (pair.size() >= 2)
                {
                    leftDescriptor = pair.at(0).trimmed();
                    rightDescriptor = pair.at(1).trimmed();
                }

                if (!leftDescriptor.isEmpty() && !fencerDescriptorsInOrder.contains(leftDescriptor))
                {
                    fencerDescriptorsInOrder.push_back(leftDescriptor);
                }
                if (!rightDescriptor.isEmpty() && !fencerDescriptorsInOrder.contains(rightDescriptor))
                {
                    fencerDescriptorsInOrder.push_back(rightDescriptor);
                }

                const QString leftName = fencerNameByDescriptor.value(leftDescriptor, leftDescriptor);
                const QString rightName = fencerNameByDescriptor.value(rightDescriptor, rightDescriptor);
                html += "<tr>";
                html += QString("<td align='center'>%1</td>").arg(boutNumber++);
                html += QString("<td>%1</td>").arg(htmlEscape(leftName));
                html += "<td>&nbsp;</td>";
                html += "<td>&nbsp;</td>";
                html += QString("<td>%1</td>").arg(htmlEscape(rightName));
                html += "</tr>";
            }
        }
        html += "</table>";

        html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
        html += "<tr><th>Fencer</th><th width='18%'>Total Hits</th><th width='32%'>Signature</th></tr>";
        for (const QString &descriptor : fencerDescriptorsInOrder)
        {
            html += "<tr>";
            html += QString("<td>%1</td>").arg(htmlEscape(fencerNameByDescriptor.value(descriptor, descriptor)));
            html += "<td>&nbsp;</td>";
            html += "<td>&nbsp;</td>";
            html += "</tr>";
        }
        html += "</table>";

        ++pageIndex;
    }

    return html;
}

QString MainWindow::bonusBracketSheetHtml() const
{
    QString html = reportHeaderHtml("Bonus Round Bracket Sheet");

    QVector<int> rankedIndexes;
    if (m_bonusRoundTab != nullptr)
    {
        rankedIndexes = m_bonusRoundTab->rankedCompetitorIndexes();
    }
    if (rankedIndexes.isEmpty() && m_fencingResultsTab != nullptr)
    {
        rankedIndexes = m_fencingResultsTab->rankedCompetitorIndexes();
    }

    QVector<int> fallback;
    const QVector<Competitor> competitors = m_eventModel.competitors();
    for (int index = 0; index < competitors.size(); ++index)
    {
        if (competitorParticipatesInDiscipline(competitors.at(index), DisciplineType::Fencing))
        {
            fallback.push_back(index);
        }
    }

    QVector<int> seeded = rankedIndexes;
    QSet<int> seen;
    QVector<int> finalSeeded;
    for (const int index : seeded)
    {
        if (index >= 0 && index < competitors.size() && !seen.contains(index)
            && competitorParticipatesInDiscipline(competitors.at(index), DisciplineType::Fencing))
        {
            finalSeeded.push_back(index);
            seen.insert(index);
        }
    }
    for (const int index : fallback)
    {
        if (!seen.contains(index))
        {
            finalSeeded.push_back(index);
            seen.insert(index);
        }
    }
    if (finalSeeded.size() > 32)
    {
        finalSeeded.resize(32);
    }

    if (finalSeeded.isEmpty())
    {
        html += "<p>No fencing participants available for a bonus round bracket.</p>";
        return html;
    }

    const int total = finalSeeded.size();
    int powerOfTwo = 1;
    while (powerOfTwo < total)
    {
        powerOfTwo *= 2;
    }
    QVector<int> seedOrder = {1, 2};
    int currentSize = 2;
    while (currentSize < powerOfTwo)
    {
        const int nextSize = currentSize * 2;
        QVector<int> expanded;
        expanded.reserve(nextSize);
        for (const int seed : seedOrder)
        {
            expanded.push_back(seed);
            expanded.push_back(nextSize + 1 - seed);
        }
        seedOrder = expanded;
        currentSize = nextSize;
    }

    struct SheetBout
    {
        int seedA = 0;
        int seedB = 0;
        int winnerSeed = 0;
        int loserSeed = 0;
        int knownCompetitorA = -1;
        int knownCompetitorB = -1;
    };

    QVector<QVector<SheetBout>> rounds;
    struct SheetEntrant
    {
        int seed = 0;
        int knownCompetitorIndex = -1;
    };

    QVector<SheetEntrant> entrants;
    entrants.reserve(seedOrder.size());
    for (const int seed : seedOrder)
    {
        if (seed > 0 && seed <= total)
        {
            entrants.push_back(SheetEntrant{seed, finalSeeded.at(seed - 1)});
        }
        else
        {
            entrants.push_back(SheetEntrant{0, -1});
        }
    }

    while (entrants.size() > 1)
    {
        QVector<SheetBout> roundBouts;
        QVector<SheetEntrant> nextEntrants;
        roundBouts.reserve(entrants.size() / 2);
        nextEntrants.reserve(entrants.size() / 2);
        for (int i = 0; i + 1 < entrants.size(); i += 2)
        {
            const SheetEntrant entrantA = entrants.at(i);
            const SheetEntrant entrantB = entrants.at(i + 1);
            const int seedA = entrantA.seed;
            const int seedB = entrantB.seed;
            int winnerSeed = 0;
            int loserSeed = 0;
            int knownWinnerCompetitor = -1;
            if (seedA == 0 && seedB == 0)
            {
                winnerSeed = 0;
                loserSeed = 0;
            }
            else if (seedA == 0)
            {
                winnerSeed = seedB;
                loserSeed = 0;
                knownWinnerCompetitor = entrantB.knownCompetitorIndex;
            }
            else if (seedB == 0)
            {
                winnerSeed = seedA;
                loserSeed = 0;
                knownWinnerCompetitor = entrantA.knownCompetitorIndex;
            }
            else
            {
                winnerSeed = std::min(seedA, seedB);
                loserSeed = std::max(seedA, seedB);
                // Bout to be fenced: winner name not known yet.
                knownWinnerCompetitor = -1;
            }

            roundBouts.push_back(SheetBout{
                seedA,
                seedB,
                winnerSeed,
                loserSeed,
                entrantA.knownCompetitorIndex,
                entrantB.knownCompetitorIndex});
            nextEntrants.push_back(SheetEntrant{winnerSeed, knownWinnerCompetitor});
        }
        rounds.push_back(roundBouts);
        entrants = nextEntrants;
    }

    html += QString("<p><strong>Bracket size:</strong> %1 (top %2 ranked fencers).</p>")
        .arg(powerOfTwo)
        .arg(total);
    html += "<p style='margin-top:4px;'>Fill in names and scores as bouts are fenced. Seed labels carry forward by branch.</p>";

    html += "<table border='1' cellspacing='0' cellpadding='3' width='100%'><tr>";
    for (int roundIndex = 0; roundIndex < rounds.size(); ++roundIndex)
    {
        const int entrantsInRound = powerOfTwo / (1 << roundIndex);
        html += QString("<th>Round of %1</th>").arg(entrantsInRound);
    }
    html += "</tr><tr>";

    for (int roundIndex = 0; roundIndex < rounds.size(); ++roundIndex)
    {
        html += "<td valign='top'><table width='100%' cellspacing='0' cellpadding='4'>";
        const QVector<SheetBout> &roundBouts = rounds.at(roundIndex);
        for (const SheetBout &bout : roundBouts)
        {
            const QString seedAText = bout.seedA > 0 ? QString::number(bout.seedA) : QString("BYE");
            const QString seedBText = bout.seedB > 0 ? QString::number(bout.seedB) : QString("BYE");
            const bool hasBye = (bout.seedA <= 0 || bout.seedB <= 0);
            const QString knownNameA = (bout.knownCompetitorA >= 0 && bout.knownCompetitorA < competitors.size())
                ? competitors.at(bout.knownCompetitorA).displayName()
                : QString();
            const QString knownNameB = (bout.knownCompetitorB >= 0 && bout.knownCompetitorB < competitors.size())
                ? competitors.at(bout.knownCompetitorB).displayName()
                : QString();

            html += "<tr><td style='border:1px solid #d9d9d9;'>";
            html += "<div style='min-height:56px; font-size:10pt;'>";
            html += QString("<div style='border-bottom:1px solid #bdbdbd; padding-bottom:2px; margin-bottom:2px;'>"
                            "<strong>(%1)</strong> %2<span style='display:inline-block; width:160px;'>&nbsp;</span>"
                            "<span style='display:inline-block; width:14px; height:14px; border:1px solid #444;'>&nbsp;</span>"
                            "</div>")
                .arg(htmlEscape(seedAText))
                .arg(htmlEscape(knownNameA.isEmpty() ? "" : knownNameA + " "));

            if (hasBye)
            {
                html += QString("<div style='padding-top:2px;'>"
                                "<strong>(%1)</strong> %2<span style='display:inline-block; width:160px;'>&nbsp;</span>"
                                "<span style='display:inline-block; width:14px; height:14px; border:1px solid #444;'>&nbsp;</span>"
                                "</div>")
                    .arg(htmlEscape(seedBText))
                    .arg(htmlEscape(knownNameB.isEmpty() ? "" : knownNameB + " "));
            }
            else
            {
                html += QString("<div style='padding-top:2px;'>"
                                "<strong>(%1)</strong> %2<span style='display:inline-block; width:160px;'>&nbsp;</span>"
                                "<span style='display:inline-block; width:14px; height:14px; border:1px solid #444;'>&nbsp;</span>"
                                "</div>")
                    .arg(htmlEscape(seedBText))
                    .arg(htmlEscape(knownNameB.isEmpty() ? "" : knownNameB + " "));
            }
            html += "</div>";
            html += "</td></tr>";

            if (roundIndex < rounds.size() - 1)
            {
                html += "<tr><td style='height:16px; border:0;'>&nbsp;</td></tr>";
            }
        }
        html += "</table></td>";
    }
    html += "</tr></table>";

    return html;
}

QString MainWindow::finalResultsHtml() const
{
    const DivisionSelection selection = m_eventModel.divisionSelection();
    const QVector<Competitor> ranked = rankedCompetitorsForFinalResults(selection);
    const EventDetails &details = m_eventModel.eventDetails();
    const QList<DisciplineType> selectedDisciplines = m_eventModel.eventDetails().selectedDisciplines.isEmpty()
        ? allDisciplines()
        : m_eventModel.eventDetails().selectedDisciplines;
    QString html;
    QString currentCombinedGroup;
    int rankInGroup = 0;
    bool firstGroup = true;
    for (const Competitor &competitor : ranked)
    {
        const QString primaryGroup = m_eventModel.divisionKeyForCompetitor(competitor, selection.primary);
        const QString secondaryGroup = m_eventModel.divisionKeyForCompetitor(competitor, selection.secondary);
        QString combinedGroup = primaryGroup;
        if (selection.secondary != ResultDivision::Overall && secondaryGroup != "Overall")
        {
            combinedGroup = QString("%1 %2").arg(primaryGroup, secondaryGroup).trimmed();
        }
        if (selection.primary == ResultDivision::Overall && selection.secondary != ResultDivision::Overall)
        {
            combinedGroup = secondaryGroup;
        }

        if (combinedGroup != currentCombinedGroup)
        {
            if (!currentCombinedGroup.isEmpty())
            {
                html += "</table>";
            }
            if (!firstGroup)
            {
                html += "<div style='page-break-before: always;'></div>";
            }
            currentCombinedGroup = combinedGroup;
            rankInGroup = 0;
            html += reportHeaderHtml(QString("Overall Results - %1").arg(combinedGroup));
            html += QString("<h2>%1</h2>").arg(htmlEscape(QString("Results - %1").arg(combinedGroup)));
            html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
            html += "<tr><th>Rank</th><th>Name</th><th>Gender</th><th>Category</th><th>Affiliation</th>";
            for (const DisciplineType discipline : selectedDisciplines)
            {
                html += QString("<th>%1 Score</th>").arg(htmlEscape(disciplineDisplayName(discipline)));
            }
            html += "<th>Total</th></tr>";
            firstGroup = false;
        }

        ++rankInGroup;
        html += "<tr>";
        html += QString("<td>%1</td>").arg(rankInGroup);
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.displayName()));
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.gender));
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.category));
        html += QString("<td>%1</td>").arg(htmlEscape(competitor.affiliation));
        for (const DisciplineType discipline : selectedDisciplines)
        {
            html += QString("<td>%1</td>").arg(htmlEscape(disciplineScoreDisplayValue(competitor, discipline)));
        }
        html += QString("<td>%1</td>").arg(m_eventModel.competitorTotalScore(competitor));
        html += "</tr>";
    }

    if (!currentCombinedGroup.isEmpty())
    {
        html += "</table>";
    }

    return html;
}

QString MainWindow::reportHeaderHtml(const QString &title) const
{
    const EventDetails &details = m_eventModel.eventDetails();
    QString html;
    html += "<table width='100%' cellspacing='0' cellpadding='0' style='margin-bottom:10px;'><tr>";
    html += "<td valign='top'>";
    html += QString("<h1 style='margin:0;'>%1</h1>").arg(htmlEscape(title));
    html += QString("<p style='margin:4px 0 0 0;'><strong>Event:</strong> %1<br><strong>Date:</strong> %2</p>")
        .arg(htmlEscape(details.eventName))
        .arg(htmlEscape(details.eventDate.toString("dd MMM yyyy")));
    html += "</td>";

    if (!details.logoPath.trimmed().isEmpty() && QFileInfo::exists(details.logoPath))
    {
        const QString logoUrl = QUrl::fromLocalFile(details.logoPath).toString();
        QImageReader reader(details.logoPath);
        const QSize logoSize = reader.size();
        int renderHeight = 60;
        int renderWidth = 180;
        if (logoSize.isValid() && logoSize.width() > 0 && logoSize.height() > 0)
        {
            renderHeight = std::min(60, logoSize.height());
            renderWidth = std::max(1, static_cast<int>(std::lround(static_cast<double>(logoSize.width()) * static_cast<double>(renderHeight) / static_cast<double>(logoSize.height()))));
        }

        html += QString("<td align='right' valign='top'>"
                        "<div style='height:60px; text-align:right;'>"
                        "<img src='%1' width='%2' height='%3' style='display:block; margin-left:auto;'/>"
                        "</div>"
                        "</td>")
            .arg(htmlEscape(logoUrl))
            .arg(renderWidth)
            .arg(renderHeight);
    }

    html += "</tr></table>";
    return html;
}

QString MainWindow::stateFilePath() const
{
    QString baseDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDirectory.isEmpty())
    {
        baseDirectory = QCoreApplication::applicationDirPath();
    }
    QDir directory(baseDirectory);
    if (!directory.exists())
    {
        directory.mkpath(".");
    }
    return directory.filePath("event-state.json");
}

void MainWindow::saveState() const
{
    if (m_loadingState)
    {
        return;
    }

    QJsonObject root;

    const EventDetails &details = m_eventModel.eventDetails();
    QJsonObject detailsObject;
    detailsObject.insert("eventName", details.eventName);
    detailsObject.insert("eventDate", details.eventDate.toString(Qt::ISODate));
    detailsObject.insert("logoPath", details.logoPath);
    detailsObject.insert("csvPath", details.csvPath);
    detailsObject.insert("rulesetId", details.rulesetId);
    QJsonArray disciplinesArray;
    for (const DisciplineType discipline : details.selectedDisciplines)
    {
        disciplinesArray.push_back(disciplineKey(discipline));
    }
    detailsObject.insert("selectedDisciplines", disciplinesArray);
    root.insert("eventDetails", detailsObject);

    const DivisionSelection &divisionSelection = m_eventModel.divisionSelection();
    QJsonObject divisionObject;
    divisionObject.insert("primary", divisionToInt(divisionSelection.primary));
    divisionObject.insert("secondary", divisionToInt(divisionSelection.secondary));
    root.insert("divisionSelection", divisionObject);

    QJsonArray competitorsArray;
    for (const Competitor &competitor : m_eventModel.competitors())
    {
        competitorsArray.push_back(competitorToJson(competitor));
    }
    root.insert("competitors", competitorsArray);
    root.insert("bonusRoundEnabled", m_bonusRoundEnabled);
    root.insert("fencingPairLimit", std::clamp(m_fencingPairLimit, 1, 10));
    QJsonObject bonusPlacingsObject;
    for (auto it = m_bonusRoundPlacingsByBib.cbegin(); it != m_bonusRoundPlacingsByBib.cend(); ++it)
    {
        bonusPlacingsObject.insert(it.key(), it.value());
    }
    root.insert("bonusRoundPlacingsByBib", bonusPlacingsObject);

    QFile file(stateFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void MainWindow::loadState()
{
    QFile file(stateFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
    {
        return;
    }

    const QJsonObject root = document.object();
    m_loadingState = true;

    EventDetails details = m_eventModel.eventDetails();
    const QJsonObject detailsObject = root.value("eventDetails").toObject();
    details.eventName = detailsObject.value("eventName").toString();
    details.eventDate = QDate::fromString(detailsObject.value("eventDate").toString(), Qt::ISODate);
    if (!details.eventDate.isValid())
    {
        details.eventDate = QDate::currentDate();
    }
    details.logoPath = detailsObject.value("logoPath").toString();
    details.csvPath = detailsObject.value("csvPath").toString();
    details.rulesetId = detailsObject.value("rulesetId").toString();
    details.selectedDisciplines.clear();
    const QJsonArray selectedDisciplines = detailsObject.value("selectedDisciplines").toArray();
    for (const QJsonValue &disciplineValue : selectedDisciplines)
    {
        const auto parsed = disciplineFromKey(disciplineValue.toString());
        if (parsed.has_value())
        {
            details.selectedDisciplines.push_back(*parsed);
        }
    }
    if (details.selectedDisciplines.isEmpty())
    {
        details.selectedDisciplines = allDisciplines();
    }

    if (details.rulesetId.isEmpty() && !m_rulesets.isEmpty())
    {
        details.rulesetId = m_rulesets.front().id;
    }

    QVector<Competitor> competitors;
    const QJsonArray competitorsArray = root.value("competitors").toArray();
    competitors.reserve(competitorsArray.size());
    for (const QJsonValue &competitorValue : competitorsArray)
    {
        if (competitorValue.isObject())
        {
            competitors.push_back(competitorFromJson(competitorValue.toObject()));
        }
    }

    DivisionSelection divisionSelection;
    const QJsonObject divisionObject = root.value("divisionSelection").toObject();
    divisionSelection.primary = divisionFromInt(divisionObject.value("primary").toInt());
    divisionSelection.secondary = divisionFromInt(divisionObject.value("secondary").toInt());
    m_bonusRoundEnabled = root.value("bonusRoundEnabled").toBool(root.value("rankingRoundEnabled").toBool(false));
    m_fencingPairLimit = std::clamp(root.value("fencingPairLimit").toInt(1), 1, 10);
    m_bonusRoundPlacingsByBib.clear();
    const QJsonObject bonusPlacingsObject = root.value("bonusRoundPlacingsByBib").toObject();
    for (auto it = bonusPlacingsObject.begin(); it != bonusPlacingsObject.end(); ++it)
    {
        if (it.value().isDouble())
        {
            m_bonusRoundPlacingsByBib.insert(it.key(), it.value().toInt());
        }
    }
    if (!m_bonusRoundPlacingsByBib.isEmpty())
    {
        m_bonusRoundEnabled = true;
    }

    m_eventModel.setEventDetails(details);
    m_eventModel.setCompetitors(competitors);
    m_eventModel.setDivisionSelection(divisionSelection);

    m_loadingState = false;
    saveState();
}

void MainWindow::printHtmlDocument(const QString &title, const QString &html) const
{
    QPrinter printer;
    QPrintDialog dialog(&printer, const_cast<MainWindow *>(this));
    dialog.setWindowTitle(title);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    QTextDocument document;
    document.setHtml(html);
    document.print(&printer);
}
