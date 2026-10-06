#pragma once

#include "csvimporter.h"
#include "bonusroundtab.h"
#include "disciplinetab.h"
#include "eventmodel.h"
#include "fencinggrouptab.h"
#include "fencingresultstab.h"
#include "finalresultstab.h"
#include "ruleset.h"
#include "setuptab.h"

#include <QMainWindow>
#include <QMap>
#include <QTabWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void applyEventDetails(const EventDetails &details);
    void importEntries(const QString &csvPath);
    void addManualCompetitor(const Competitor &competitor);
    void updateCompetitorFromPreview(int competitorIndex, const Competitor &competitor);
    void deleteCompetitorsFromPreview(const QVector<int> &competitorIndexes);
    void autoAssignNumbersFromPreview(const QVector<int> &competitorIndexesInViewOrder, int startingNumber);
    void assignHeats(DisciplineType discipline, int lanesPerHeat);
    void autoAssignFencingGroups(int numberPistes);
    void exportFencingGroupListPdf();
    void exportFencingRoundSheetsPdf();
    void exportBonusBracketSheetPdf();
    void updateLaneAssignment(DisciplineType discipline, int competitorIndex, int heat, int lane);
    void updateRawResult(DisciplineType discipline, int competitorIndex, const QString &rawResult);
    void updatePenalty(DisciplineType discipline, int competitorIndex, const QString &penalty);
    void recalculateDiscipline(DisciplineType discipline);
    void printParticipantSummaryPdf();
    void printBlankSheet(DisciplineType discipline);
    void printDisciplineResults(DisciplineType discipline);
    void printFinalResults();
    void exportEventCsv();
    void clearEventData();
    void refreshViews();
    void updateFencingRawResult(int competitorIndex, const QString &rawResult);
    void updateFencingPenalty(int competitorIndex, const QString &penalty);
    void recalculateFencingResults();
    void setBonusRoundEnabled(bool enabled);
    void updateBonusRoundPlacings(const QMap<int, int> &placementsByCompetitorIndex);
    void handleTabChanged(int index);

private:
    const Ruleset *currentRuleset() const;
    void refreshDisciplineTabs();
    QString disciplineTableHtml(DisciplineType discipline, bool includeResults) const;
    QString fencingGroupListHtml() const;
    QString fencingRoundSheetsHtml() const;
    QString bonusBracketSheetHtml() const;
    QString participantSummaryHtml() const;
    QString finalResultsHtml() const;
    QString reportHeaderHtml(const QString &title) const;
    void printHtmlDocument(const QString &title, const QString &html) const;
    int pairLimitForFencing() const;
    QVector<Competitor> competitorsForFinalResults() const;
    QVector<Competitor> rankedCompetitorsForFinalResults(const DivisionSelection &selection) const;
    int computedFencingRankingScore(const Competitor &competitor) const;
    int computedFencingBonusScore(const Competitor &competitor) const;
    void recalculateFencingRankingScores();
    int computedScoreForCompetitor(const Competitor &competitor, DisciplineType discipline, const DisciplineRule *rule) const;
    void recalculateCompetitorScores(int competitorIndex);
    bool competitorHasDsq(const Competitor &competitor) const;
    QString stateFilePath() const;
    void saveState() const;
    void loadState();

    EventModel m_eventModel;
    CsvImporter m_csvImporter;
    RulesetRepository m_rulesetRepository;
    QVector<Ruleset> m_rulesets;

    QTabWidget *m_tabWidget = nullptr;
    SetupTab *m_setupTab = nullptr;
    FinalResultsTab *m_finalResultsTab = nullptr;
    FencingGroupTab *m_fencingGroupTab = nullptr;
    FencingResultsTab *m_fencingResultsTab = nullptr;
    BonusRoundTab *m_bonusRoundTab = nullptr;
    QMap<DisciplineType, DisciplineTab *> m_disciplineTabs;
    bool m_loadingState = false;
    bool m_bonusRoundEnabled = false;
    QMap<QString, int> m_bonusRoundPlacingsByBib;
    int m_fencingPairLimit = 1;
    bool m_pendingFencingRecalculation = false;
    QWidget *m_lastActiveTab = nullptr;
};
