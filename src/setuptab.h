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
    void exportCsvRequested();
    void clearAllRequested();

private slots:
    void chooseLogo();
    void chooseCsv();
    void submitDetails();
    void submitManualCompetitor();

private:
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
    QLineEdit *m_manualCategoryEdit = nullptr;
    QLineEdit *m_manualAffiliationEdit = nullptr;
    QDateEdit *m_manualDobEdit = nullptr;
    QTableWidget *m_previewTable = nullptr;
};
