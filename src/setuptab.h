#pragma once

#include "eventmodel.h"
#include "ruleset.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

class SetupTab : public QWidget
{
    Q_OBJECT

public:
    explicit SetupTab(QWidget *parent = nullptr);

    void setRulesets(const QVector<Ruleset> &rulesets);
    void setEventDetails(const EventDetails &details);
    void setCompetitorPreview(const QVector<Competitor> &competitors);

signals:
    void eventDetailsSubmitted(const EventDetails &details);
    void importEntriesRequested(const QString &csvPath);
    void manualCompetitorSubmitted(const Competitor &competitor);
    void competitorPreviewEdited(int competitorIndex, const Competitor &competitor);
    void deleteCompetitorsRequested(const QVector<int> &competitorIndexes);
    void autoAssignNumbersRequested(const QVector<int> &competitorIndexesInViewOrder, int startingNumber);
    void printParticipantSummaryRequested();
    void exportCsvRequested();
    void clearAllRequested();

private slots:
    void chooseLogo();
    void chooseCsv();
    void submitDetails();
    void submitManualCompetitor();
    void handlePreviewCellChanged(int row, int column);
    void requestDeleteSelectedRows();
    void requestAutoAssignNumbers();

private:
    bool tryParseBirthYear(const QString &text, int *birthYear) const;
    EventDetails currentDetailsFromForm() const;
    Competitor currentManualCompetitorFromForm() const;
    void clearManualCompetitorForm();

    QLineEdit *m_eventNameEdit = nullptr;
    QDateEdit *m_dateEdit = nullptr;
    QLineEdit *m_logoEdit = nullptr;
    QLineEdit *m_csvEdit = nullptr;
    QComboBox *m_rulesetCombo = nullptr;
    QMap<DisciplineType, QCheckBox *> m_disciplineChecks;
    QLineEdit *m_manualBibEdit = nullptr;
    QLineEdit *m_manualFirstNameEdit = nullptr;
    QLineEdit *m_manualLastNameEdit = nullptr;
    QLineEdit *m_manualGenderEdit = nullptr;
    QComboBox *m_manualCategoryOverrideCombo = nullptr;
    QLineEdit *m_manualAffiliationEdit = nullptr;
    QSpinBox *m_manualBirthYearSpin = nullptr;
    QSpinBox *m_autoNumberStartSpin = nullptr;
    QTableWidget *m_previewTable = nullptr;
    QVector<Competitor> m_previewCompetitors;
    bool m_updatingPreview = false;
    int m_eventYear = QDate::currentDate().year();
};
