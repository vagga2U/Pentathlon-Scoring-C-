#include "setuptab.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

SetupTab::SetupTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *detailsGroup = new QGroupBox("Event Setup", this);
    auto *detailsLayout = new QFormLayout(detailsGroup);

    m_eventNameEdit = new QLineEdit(this);
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);

    auto *logoRow = new QWidget(this);
    auto *logoLayout = new QHBoxLayout(logoRow);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    m_logoEdit = new QLineEdit(this);
    auto *logoButton = new QPushButton("Browse...", this);
    logoLayout->addWidget(m_logoEdit);
    logoLayout->addWidget(logoButton);

    auto *csvRow = new QWidget(this);
    auto *csvLayout = new QHBoxLayout(csvRow);
    csvLayout->setContentsMargins(0, 0, 0, 0);
    m_csvEdit = new QLineEdit(this);
    auto *csvButton = new QPushButton("Import CSV...", this);
    csvLayout->addWidget(m_csvEdit);
    csvLayout->addWidget(csvButton);

    m_rulesetCombo = new QComboBox(this);

    detailsLayout->addRow("Event Name", m_eventNameEdit);
    detailsLayout->addRow("Event Date", m_dateEdit);
    detailsLayout->addRow("Logo", logoRow);
    detailsLayout->addRow("Entries CSV", csvRow);
    detailsLayout->addRow("Ruleset", m_rulesetCombo);

    auto *disciplineGroup = new QGroupBox("Included Disciplines", this);
    auto *disciplineLayout = new QGridLayout(disciplineGroup);
    int column = 0;
    for (const DisciplineType discipline : allDisciplines())
    {
        auto *check = new QCheckBox(disciplineDisplayName(discipline), this);
        check->setChecked(true);
        disciplineLayout->addWidget(check, 0, column++);
        m_disciplineChecks.insert(discipline, check);
    }

    auto *buttonRow = new QWidget(this);
    auto *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    auto *exportButton = new QPushButton("Export Event CSV...", this);
    auto *clearButton = new QPushButton("Clear Event Data...", this);
    auto *applyButton = new QPushButton("Apply Event Details", this);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);

    auto *manualEntryGroup = new QGroupBox("Manual Competitor Entry", this);
    auto *manualEntryLayout = new QGridLayout(manualEntryGroup);
    m_manualBibEdit = new QLineEdit(this);
    m_manualFirstNameEdit = new QLineEdit(this);
    m_manualLastNameEdit = new QLineEdit(this);
    m_manualGenderEdit = new QLineEdit(this);
    m_manualCategoryEdit = new QLineEdit(this);
    m_manualAffiliationEdit = new QLineEdit(this);
    m_manualDobEdit = new QDateEdit(QDate::currentDate(), this);
    m_manualDobEdit->setCalendarPopup(true);
    auto *manualAddButton = new QPushButton("Add Competitor", this);

    manualEntryLayout->addWidget(new QLabel("Number", this), 0, 0);
    manualEntryLayout->addWidget(m_manualBibEdit, 0, 1);
    manualEntryLayout->addWidget(new QLabel("First Name", this), 0, 2);
    manualEntryLayout->addWidget(m_manualFirstNameEdit, 0, 3);
    manualEntryLayout->addWidget(new QLabel("Last Name", this), 0, 4);
    manualEntryLayout->addWidget(m_manualLastNameEdit, 0, 5);
    manualEntryLayout->addWidget(new QLabel("Gender", this), 1, 0);
    manualEntryLayout->addWidget(m_manualGenderEdit, 1, 1);
    manualEntryLayout->addWidget(new QLabel("Category", this), 1, 2);
    manualEntryLayout->addWidget(m_manualCategoryEdit, 1, 3);
    manualEntryLayout->addWidget(new QLabel("Affiliation", this), 1, 4);
    manualEntryLayout->addWidget(m_manualAffiliationEdit, 1, 5);
    manualEntryLayout->addWidget(new QLabel("Date of Birth", this), 2, 0);
    manualEntryLayout->addWidget(m_manualDobEdit, 2, 1);
    manualEntryLayout->addWidget(manualAddButton, 2, 5);

    m_previewTable = new QTableWidget(this);
    m_previewTable->setColumnCount(6);
    m_previewTable->setHorizontalHeaderLabels({"Number", "Name", "Gender", "Category", "Affiliation", "DOB"});
    m_previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(detailsGroup);
    layout->addWidget(disciplineGroup);
    layout->addWidget(buttonRow);
    layout->addWidget(manualEntryGroup);
    layout->addWidget(new QLabel("Entries Preview", this));
    layout->addWidget(m_previewTable, 1);

    connect(logoButton, &QPushButton::clicked, this, &SetupTab::chooseLogo);
    connect(csvButton, &QPushButton::clicked, this, &SetupTab::chooseCsv);
    connect(exportButton, &QPushButton::clicked, this, &SetupTab::exportCsvRequested);
    connect(clearButton, &QPushButton::clicked, this, &SetupTab::clearAllRequested);
    connect(applyButton, &QPushButton::clicked, this, &SetupTab::submitDetails);
    connect(manualAddButton, &QPushButton::clicked, this, &SetupTab::submitManualCompetitor);
}

void SetupTab::setRulesets(const QVector<Ruleset> &rulesets)
{
    m_rulesetCombo->clear();
    for (const Ruleset &ruleset : rulesets)
    {
        m_rulesetCombo->addItem(ruleset.name, ruleset.id);
    }
}

void SetupTab::setEventDetails(const EventDetails &details)
{
    m_eventNameEdit->setText(details.eventName);
    m_dateEdit->setDate(details.eventDate.isValid() ? details.eventDate : QDate::currentDate());
    m_logoEdit->setText(details.logoPath);
    m_csvEdit->setText(details.csvPath);

    const int rulesetIndex = m_rulesetCombo->findData(details.rulesetId);
    if (rulesetIndex >= 0)
    {
        m_rulesetCombo->setCurrentIndex(rulesetIndex);
    }

    for (auto it = m_disciplineChecks.begin(); it != m_disciplineChecks.end(); ++it)
    {
        it.value()->setChecked(details.selectedDisciplines.contains(it.key()));
    }
}

void SetupTab::setCompetitorPreview(const QVector<Competitor> &competitors)
{
    m_previewTable->setRowCount(competitors.size());
    for (int row = 0; row < competitors.size(); ++row)
    {
        const Competitor &competitor = competitors.at(row);
        m_previewTable->setItem(row, 0, new QTableWidgetItem(competitor.bib));
        m_previewTable->setItem(row, 1, new QTableWidgetItem(competitor.displayName()));
        m_previewTable->setItem(row, 2, new QTableWidgetItem(competitor.gender));
        m_previewTable->setItem(row, 3, new QTableWidgetItem(competitor.category));
        m_previewTable->setItem(row, 4, new QTableWidgetItem(competitor.affiliation));
        m_previewTable->setItem(row, 5, new QTableWidgetItem(competitor.dateOfBirth.isValid() ? competitor.dateOfBirth.toString("dd/MM/yyyy") : ""));
    }
}

void SetupTab::chooseLogo()
{
    const QString path = QFileDialog::getOpenFileName(this, "Select Event Logo", {}, "Images (*.png *.jpg *.jpeg *.bmp *.svg)");
    if (!path.isEmpty())
    {
        m_logoEdit->setText(path);
    }
}

void SetupTab::chooseCsv()
{
    const QString path = QFileDialog::getOpenFileName(this, "Select Entries CSV", {}, "CSV Files (*.csv)");
    if (!path.isEmpty())
    {
        m_csvEdit->setText(path);
        emit importEntriesRequested(path);
    }
}

void SetupTab::submitDetails()
{
    emit eventDetailsSubmitted(currentDetailsFromForm());
}

void SetupTab::submitManualCompetitor()
{
    emit manualCompetitorSubmitted(currentManualCompetitorFromForm());
    clearManualCompetitorForm();
}

EventDetails SetupTab::currentDetailsFromForm() const
{
    EventDetails details;
    details.eventName = m_eventNameEdit->text().trimmed();
    details.eventDate = m_dateEdit->date();
    details.logoPath = m_logoEdit->text().trimmed();
    details.csvPath = m_csvEdit->text().trimmed();
    details.rulesetId = m_rulesetCombo->currentData().toString();

    details.selectedDisciplines.clear();
    for (auto it = m_disciplineChecks.cbegin(); it != m_disciplineChecks.cend(); ++it)
    {
        if (it.value()->isChecked())
        {
            details.selectedDisciplines.push_back(it.key());
        }
    }

    return details;
}

Competitor SetupTab::currentManualCompetitorFromForm() const
{
    Competitor competitor;
    competitor.bib = m_manualBibEdit->text().trimmed();
    competitor.firstName = m_manualFirstNameEdit->text().trimmed();
    competitor.lastName = m_manualLastNameEdit->text().trimmed();
    competitor.gender = m_manualGenderEdit->text().trimmed();
    competitor.category = m_manualCategoryEdit->text().trimmed();
    competitor.affiliation = m_manualAffiliationEdit->text().trimmed();
    competitor.dateOfBirth = m_manualDobEdit->date();
    return competitor;
}

void SetupTab::clearManualCompetitorForm()
{
    m_manualBibEdit->clear();
    m_manualFirstNameEdit->clear();
    m_manualLastNameEdit->clear();
    m_manualGenderEdit->clear();
    m_manualCategoryEdit->clear();
    m_manualAffiliationEdit->clear();
    m_manualDobEdit->setDate(QDate::currentDate());
}
