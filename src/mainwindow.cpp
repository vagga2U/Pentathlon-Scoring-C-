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
    object.insert("affiliation", competitor.affiliation);
    object.insert("dateOfBirth", competitor.dateOfBirth.isValid() ? competitor.dateOfBirth.toString(Qt::ISODate) : QString());
    object.insert("fencingGroup", competitor.fencingGroup);

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
    competitor.affiliation = object.value("affiliation").toString();
    competitor.dateOfBirth = QDate::fromString(object.value("dateOfBirth").toString(), Qt::ISODate);
    competitor.fencingGroup = object.value("fencingGroup").toInt();

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

    connect(m_setupTab, &SetupTab::eventDetailsSubmitted, this, &MainWindow::applyEventDetails);
    connect(m_setupTab, &SetupTab::importEntriesRequested, this, &MainWindow::importEntries);
    connect(m_setupTab, &SetupTab::manualCompetitorSubmitted, this, &MainWindow::addManualCompetitor);
    connect(m_setupTab, &SetupTab::exportCsvRequested, this, &MainWindow::exportEventCsv);
    connect(m_setupTab, &SetupTab::clearAllRequested, this, &MainWindow::clearEventData);
    connect(&m_eventModel, &EventModel::changed, this, &MainWindow::refreshViews);
    connect(&m_eventModel, &EventModel::changed, this, &MainWindow::saveState);
    connect(m_finalResultsTab, &FinalResultsTab::divisionSelectionChanged, &m_eventModel, &EventModel::setDivisionSelection);
    connect(m_finalResultsTab, &FinalResultsTab::printRequested, this, &MainWindow::printFinalResults);

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
        m_fencingGroupTab->setStatusMessage(
            QString("Assigned %1 fencers into %2 group(s) using %3 piste(s).")
                .arg(assignedCount)
                .arg(highestGroup)
                .arg(numberPistes),
            false);
    }
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

int MainWindow::computedScoreForCompetitor(const Competitor &competitor, DisciplineType discipline, const DisciplineRule *rule) const
{
    if (competitorHasDsq(competitor))
    {
        return 0;
    }

    const QString penalty = competitor.penalties.value(discipline).trimmed();
    if (isElmPenalty(penalty))
    {
        return 0;
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
        "event_name", "event_date", "ruleset", "number", "first_name", "last_name", "gender", "category", "affiliation", "dob"
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

    for (const Competitor &competitor : m_eventModel.competitors())
    {
        QStringList row = {
            csvEscape(details.eventName),
            csvEscape(details.eventDate.isValid() ? details.eventDate.toString(Qt::ISODate) : QString()),
            csvEscape(details.rulesetId),
            csvEscape(competitor.bib),
            csvEscape(competitor.firstName),
            csvEscape(competitor.lastName),
            csvEscape(competitor.gender),
            csvEscape(competitor.category),
            csvEscape(competitor.affiliation),
            csvEscape(competitor.dateOfBirth.isValid() ? competitor.dateOfBirth.toString(Qt::ISODate) : QString())
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
    refreshDisciplineTabs();
    refreshViews();

    QMessageBox::information(this, "Clear Event Data", "Event data has been cleared.");
}

void MainWindow::refreshViews()
{
    m_setupTab->setEventDetails(m_eventModel.eventDetails());
    m_setupTab->setCompetitorPreview(m_eventModel.competitors());
    m_finalResultsTab->setDivisionSelection(m_eventModel.divisionSelection());
    m_finalResultsTab->populate(m_eventModel.rankedCompetitors(), m_eventModel, m_eventModel.eventDetails().selectedDisciplines);

    const Ruleset *ruleset = currentRuleset();
    for (auto it = m_disciplineTabs.begin(); it != m_disciplineTabs.end(); ++it)
    {
        it.value()->setRuleSummary(ruleset != nullptr ? ruleset->ruleFor(it.key()) : nullptr);
        it.value()->populate(m_eventModel.competitors());
    }

    if (m_fencingGroupTab != nullptr)
    {
        m_fencingGroupTab->populate(m_eventModel.competitors());
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
        m_tabWidget->insertTab(insertIndex++, m_fencingGroupTab, "Fencing Groups");
    }

    for (const DisciplineType discipline : disciplines)
    {
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

QString MainWindow::finalResultsHtml() const
{
    const DivisionSelection selection = m_eventModel.divisionSelection();
    const QVector<Competitor> ranked = m_eventModel.rankedCompetitors(selection);
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
