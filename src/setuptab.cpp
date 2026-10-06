#include "setuptab.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QColor>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSet>
#include <QVBoxLayout>

namespace
{
QStringList allowedCategories()
{
    return {
        "U9",
        "U11",
        "U13",
        "U15",
        "U17",
        "U19",
        "Juniors",
        "Seniors",
        "Masters 30+",
        "Masters 40+",
        "Masters 50+",
        "Masters 60+",
        "Masters 70+"
    };
}

QString canonicalCategory(const QString &category)
{
    const QString trimmed = category.trimmed();
    if (trimmed.isEmpty())
    {
        return {};
    }

    for (const QString &allowed : allowedCategories())
    {
        if (trimmed.compare(allowed, Qt::CaseInsensitive) == 0)
        {
            return allowed;
        }
    }
    return {};
}

QString categorySortKey(const QString &category)
{
    const QString normalizedCategory = canonicalCategory(category);
    const QString normalized = normalizedCategory.isEmpty() ? category.trimmed().toLower() : normalizedCategory.toLower();
    if (normalized.isEmpty())
    {
        return "9-999-";
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
}

class PreviewTableItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;

    bool operator<(const QTableWidgetItem &other) const override
    {
        const QString leftSortKey = data(Qt::UserRole + 1).toString();
        const QString rightSortKey = other.data(Qt::UserRole + 1).toString();
        if (!leftSortKey.isEmpty() || !rightSortKey.isEmpty())
        {
            return leftSortKey < rightSortKey;
        }

        return QTableWidgetItem::operator<(other);
    }
};

PreviewTableItem *previewItem(const QString &displayValue, const QString &sortKey = QString())
{
    auto *item = new PreviewTableItem(displayValue);
    if (!sortKey.isEmpty())
    {
        item->setData(Qt::UserRole + 1, sortKey);
    }
    return item;
}

QString birthYearSortKey(int birthYear)
{
    if (birthYear <= 0)
    {
        return "99999";
    }
    return QString("%1").arg(birthYear, 5, 10, QChar('0'));
}

QString bibSortKey(const QString &bib)
{
    const QString trimmed = bib.trimmed();
    bool ok = false;
    const int numericBib = trimmed.toInt(&ok);
    if (ok)
    {
        return QString("0-%1").arg(numericBib, 10, 10, QChar('0'));
    }
    return QString("1-%1").arg(trimmed.toLower());
}

QString categoryFromBirthYear(int birthYear, int competitionYear)
{
    if (birthYear <= 0 || competitionYear <= 0 || birthYear > competitionYear)
    {
        return "Uncategorised";
    }

    const int age = competitionYear - birthYear;
    if (age <= 8)
    {
        return "U9";
    }
    if (age <= 10)
    {
        return "U11";
    }
    if (age <= 12)
    {
        return "U13";
    }
    if (age <= 14)
    {
        return "U15";
    }
    if (age <= 16)
    {
        return "U17";
    }
    if (age <= 18)
    {
        return "U19";
    }
    if (age <= 21)
    {
        return "Juniors";
    }
    if (age <= 29)
    {
        return "Seniors";
    }
    if (age <= 39)
    {
        return "Masters 30+";
    }
    if (age <= 49)
    {
        return "Masters 40+";
    }
    if (age <= 59)
    {
        return "Masters 50+";
    }
    if (age <= 69)
    {
        return "Masters 60+";
    }
    return "Masters 70+";
}
}

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
    auto *printSummaryButton = new QPushButton("Print Participant Summary (PDF)...", this);
    auto *exportButton = new QPushButton("Export Event CSV...", this);
    auto *clearButton = new QPushButton("Clear Event Data...", this);
    auto *applyButton = new QPushButton("Apply Event Details", this);
    buttonLayout->addWidget(printSummaryButton);
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
    m_manualCategoryOverrideCombo = new QComboBox(this);
    m_manualCategoryOverrideCombo->addItem("Auto");
    for (const QString &category : allowedCategories())
    {
        m_manualCategoryOverrideCombo->addItem(category);
    }
    m_manualAffiliationEdit = new QLineEdit(this);
    m_manualBirthYearSpin = new QSpinBox(this);
    m_manualBirthYearSpin->setRange(1900, 2200);
    m_manualBirthYearSpin->setValue(QDate::currentDate().year());
    auto *manualAddButton = new QPushButton("Add Competitor", this);

    manualEntryLayout->addWidget(new QLabel("Number", this), 0, 0);
    manualEntryLayout->addWidget(m_manualBibEdit, 0, 1);
    manualEntryLayout->addWidget(new QLabel("First Name", this), 0, 2);
    manualEntryLayout->addWidget(m_manualFirstNameEdit, 0, 3);
    manualEntryLayout->addWidget(new QLabel("Last Name", this), 0, 4);
    manualEntryLayout->addWidget(m_manualLastNameEdit, 0, 5);
    manualEntryLayout->addWidget(new QLabel("Gender", this), 1, 0);
    manualEntryLayout->addWidget(m_manualGenderEdit, 1, 1);
    manualEntryLayout->addWidget(new QLabel("Birth Year", this), 1, 2);
    manualEntryLayout->addWidget(m_manualBirthYearSpin, 1, 3);
    manualEntryLayout->addWidget(new QLabel("Affiliation", this), 1, 4);
    manualEntryLayout->addWidget(m_manualAffiliationEdit, 1, 5);
    manualEntryLayout->addWidget(new QLabel("Category Override (optional)", this), 2, 0);
    manualEntryLayout->addWidget(m_manualCategoryOverrideCombo, 2, 1, 1, 2);
    manualEntryLayout->addWidget(new QLabel("Select Auto to use event year + birth year category", this), 2, 3, 1, 2);
    manualEntryLayout->addWidget(manualAddButton, 2, 5);

    m_previewTable = new QTableWidget(this);
    m_previewTable->setColumnCount(11);
    m_previewTable->setHorizontalHeaderLabels({
        "Number",
        "First Name",
        "Last Name",
        "Gender",
        "Birth Year",
        "Category",
        "Affiliation",
        "Fence",
        "Obstacle",
        "Swim",
        "Laser Run"});
    m_previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_previewTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    m_previewTable->setSortingEnabled(true);

    auto *previewToolbar = new QWidget(this);
    auto *previewToolbarLayout = new QHBoxLayout(previewToolbar);
    previewToolbarLayout->setContentsMargins(0, 0, 0, 0);
    auto *deleteRowsButton = new QPushButton("Delete Selected Rows", this);
    auto *autoNumberButton = new QPushButton("Auto Assign Numbers", this);
    m_autoNumberStartSpin = new QSpinBox(this);
    m_autoNumberStartSpin->setRange(1, 99999);
    m_autoNumberStartSpin->setValue(1);
    previewToolbarLayout->addWidget(deleteRowsButton);
    previewToolbarLayout->addWidget(autoNumberButton);
    previewToolbarLayout->addWidget(new QLabel("Starting at", this));
    previewToolbarLayout->addWidget(m_autoNumberStartSpin);
    previewToolbarLayout->addStretch();

    layout->addWidget(detailsGroup);
    layout->addWidget(disciplineGroup);
    layout->addWidget(buttonRow);
    layout->addWidget(manualEntryGroup);
    layout->addWidget(new QLabel("Entries Preview", this));
    layout->addWidget(previewToolbar);
    layout->addWidget(m_previewTable, 1);

    connect(logoButton, &QPushButton::clicked, this, &SetupTab::chooseLogo);
    connect(csvButton, &QPushButton::clicked, this, &SetupTab::chooseCsv);
    connect(printSummaryButton, &QPushButton::clicked, this, &SetupTab::printParticipantSummaryRequested);
    connect(exportButton, &QPushButton::clicked, this, &SetupTab::exportCsvRequested);
    connect(clearButton, &QPushButton::clicked, this, &SetupTab::clearAllRequested);
    connect(applyButton, &QPushButton::clicked, this, &SetupTab::submitDetails);
    connect(manualAddButton, &QPushButton::clicked, this, &SetupTab::submitManualCompetitor);
    connect(deleteRowsButton, &QPushButton::clicked, this, &SetupTab::requestDeleteSelectedRows);
    connect(autoNumberButton, &QPushButton::clicked, this, &SetupTab::requestAutoAssignNumbers);
    connect(m_previewTable, &QTableWidget::cellChanged, this, &SetupTab::handlePreviewCellChanged);
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
    m_eventYear = details.eventDate.isValid() ? details.eventDate.year() : QDate::currentDate().year();
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
    m_updatingPreview = true;
    m_previewCompetitors = competitors;
    m_previewTable->setSortingEnabled(false);
    m_previewTable->clearContents();
    m_previewTable->setRowCount(competitors.size());
    for (int row = 0; row < competitors.size(); ++row)
    {
        const Competitor &competitor = competitors.at(row);
        auto *numberItem = previewItem(competitor.bib, bibSortKey(competitor.bib));
        numberItem->setData(Qt::UserRole, row);
        m_previewTable->setItem(row, 0, numberItem);
        m_previewTable->setItem(row, 1, previewItem(competitor.firstName));
        m_previewTable->setItem(row, 2, previewItem(competitor.lastName));
        m_previewTable->setItem(row, 3, previewItem(competitor.gender));
        m_previewTable->setItem(row, 4, previewItem(competitor.birthYear > 0 ? QString::number(competitor.birthYear) : QString(), birthYearSortKey(competitor.birthYear)));
        m_previewTable->setItem(row, 5, previewItem(competitor.category, categorySortKey(competitor.category)));
        m_previewTable->setItem(row, 6, previewItem(competitor.affiliation));
        const QList<DisciplineType> disciplines = {DisciplineType::Fencing, DisciplineType::Obstacle, DisciplineType::Swimming, DisciplineType::LaserRun};
        for (int disciplineOffset = 0; disciplineOffset < disciplines.size(); ++disciplineOffset)
        {
            auto *participationItem = previewItem({});
            participationItem->setFlags((participationItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled) & ~Qt::ItemIsEditable);
            participationItem->setCheckState(competitorParticipatesInDiscipline(competitor, disciplines.at(disciplineOffset)) ? Qt::Checked : Qt::Unchecked);
            m_previewTable->setItem(row, 7 + disciplineOffset, participationItem);
        }
        if (m_previewTable->item(row, 5) != nullptr)
        {
            const QString autoCategory = categoryFromBirthYear(competitor.birthYear, m_eventYear);
            const bool isOverride = competitor.categoryOverride && competitor.category.compare(autoCategory, Qt::CaseInsensitive) != 0;
            m_previewTable->item(row, 5)->setData(Qt::BackgroundRole, isOverride ? QVariant(QColor("#fff4cc")) : QVariant());
            m_previewTable->item(row, 5)->setData(Qt::ForegroundRole, isOverride ? QVariant(QColor("#000000")) : QVariant());
            m_previewTable->item(row, 5)->setToolTip(isOverride ? QString("Manual override (auto would be %1)").arg(autoCategory) : QString("Auto category from birth year"));
        }
    }
    m_previewTable->setSortingEnabled(true);
    m_updatingPreview = false;
}

void SetupTab::handlePreviewCellChanged(int row, int)
{
    if (m_updatingPreview)
    {
        return;
    }

    QTableWidgetItem *numberItem = m_previewTable->item(row, 0);
    if (numberItem == nullptr)
    {
        return;
    }

    bool indexOk = false;
    const int competitorIndex = numberItem->data(Qt::UserRole).toInt(&indexOk);
    if (!indexOk || competitorIndex < 0 || competitorIndex >= m_previewCompetitors.size())
    {
        return;
    }

    Competitor updated = m_previewCompetitors.at(competitorIndex);
    updated.bib = m_previewTable->item(row, 0) != nullptr ? m_previewTable->item(row, 0)->text().trimmed() : QString();
    updated.firstName = m_previewTable->item(row, 1) != nullptr ? m_previewTable->item(row, 1)->text().trimmed() : QString();
    updated.lastName = m_previewTable->item(row, 2) != nullptr ? m_previewTable->item(row, 2)->text().trimmed() : QString();
    updated.gender = m_previewTable->item(row, 3) != nullptr ? m_previewTable->item(row, 3)->text().trimmed() : QString();
    const QString birthYearText = m_previewTable->item(row, 4) != nullptr ? m_previewTable->item(row, 4)->text().trimmed() : QString();
    int parsedBirthYear = 0;
    if (!tryParseBirthYear(birthYearText, &parsedBirthYear))
    {
        QMessageBox::warning(this, "Entries Preview", "Invalid birth year. Please enter a 4-digit year.");
        m_updatingPreview = true;
        m_previewTable->item(row, 4)->setText(updated.birthYear > 0 ? QString::number(updated.birthYear) : "");
        m_updatingPreview = false;
        return;
    }
    updated.birthYear = parsedBirthYear;
    const QString categoryText = m_previewTable->item(row, 5) != nullptr ? m_previewTable->item(row, 5)->text().trimmed() : QString();
    const QString canonicalInputCategory = canonicalCategory(categoryText);
    if (!categoryText.isEmpty() && canonicalInputCategory.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Entries Preview",
            QString("Invalid category. Allowed categories are:\n%1").arg(allowedCategories().join(", ")));
        m_updatingPreview = true;
        if (m_previewTable->item(row, 5) != nullptr)
        {
            m_previewTable->item(row, 5)->setText(updated.category);
        }
        m_updatingPreview = false;
        return;
    }

    const QString autoCategory = categoryFromBirthYear(updated.birthYear, m_eventYear);
    if (categoryText.isEmpty() || canonicalInputCategory.compare(autoCategory, Qt::CaseInsensitive) == 0)
    {
        updated.categoryOverride = false;
        updated.category = autoCategory;
    }
    else
    {
        updated.categoryOverride = true;
        updated.category = canonicalInputCategory;
    }
    updated.affiliation = m_previewTable->item(row, 6) != nullptr ? m_previewTable->item(row, 6)->text().trimmed() : QString();
    const QList<DisciplineType> disciplines = {DisciplineType::Fencing, DisciplineType::Obstacle, DisciplineType::Swimming, DisciplineType::LaserRun};
    for (int disciplineOffset = 0; disciplineOffset < disciplines.size(); ++disciplineOffset)
    {
        const int column = 7 + disciplineOffset;
        QTableWidgetItem *participationItem = m_previewTable->item(row, column);
        const bool enabled = participationItem != nullptr && participationItem->checkState() == Qt::Checked;
        updated.disciplineParticipation.insert(disciplines.at(disciplineOffset), enabled);
    }

    if (m_previewTable->item(row, 4) != nullptr)
    {
        m_previewTable->item(row, 4)->setData(Qt::UserRole + 1, birthYearSortKey(updated.birthYear));
    }
    if (m_previewTable->item(row, 0) != nullptr)
    {
        m_previewTable->item(row, 0)->setData(Qt::UserRole + 1, bibSortKey(updated.bib));
    }
    if (m_previewTable->item(row, 5) != nullptr)
    {
        m_previewTable->item(row, 5)->setText(updated.category);
        m_previewTable->item(row, 5)->setData(Qt::UserRole + 1, categorySortKey(updated.category));
        m_previewTable->item(row, 5)->setData(Qt::BackgroundRole, updated.categoryOverride ? QVariant(QColor("#fff4cc")) : QVariant());
        m_previewTable->item(row, 5)->setData(Qt::ForegroundRole, updated.categoryOverride ? QVariant(QColor("#000000")) : QVariant());
        m_previewTable->item(row, 5)->setToolTip(updated.categoryOverride ? QString("Manual override (auto would be %1)").arg(autoCategory) : QString("Auto category from birth year"));
    }

    if (updated.displayName().isEmpty())
    {
        QMessageBox::warning(this, "Entries Preview", "First name or last name is required.");
        m_updatingPreview = true;
        m_previewTable->item(row, 1)->setText(m_previewCompetitors.at(competitorIndex).firstName);
        m_previewTable->item(row, 2)->setText(m_previewCompetitors.at(competitorIndex).lastName);
        m_updatingPreview = false;
        return;
    }

    m_previewCompetitors[competitorIndex] = updated;
    emit competitorPreviewEdited(competitorIndex, updated);
}

void SetupTab::requestAutoAssignNumbers()
{
    QVector<int> competitorIndexesInViewOrder;
    competitorIndexesInViewOrder.reserve(m_previewTable->rowCount());

    for (int row = 0; row < m_previewTable->rowCount(); ++row)
    {
        QTableWidgetItem *numberItem = m_previewTable->item(row, 0);
        if (numberItem == nullptr)
        {
            continue;
        }

        bool indexOk = false;
        const int competitorIndex = numberItem->data(Qt::UserRole).toInt(&indexOk);
        if (indexOk)
        {
            competitorIndexesInViewOrder.push_back(competitorIndex);
        }
    }

    emit autoAssignNumbersRequested(competitorIndexesInViewOrder, m_autoNumberStartSpin->value());
}

void SetupTab::requestDeleteSelectedRows()
{
    const QList<QTableWidgetItem *> selectedItems = m_previewTable->selectedItems();
    QSet<int> rowIndexes;
    for (QTableWidgetItem *item : selectedItems)
    {
        if (item != nullptr)
        {
            rowIndexes.insert(item->row());
        }
    }

    QVector<int> competitorIndexes;
    competitorIndexes.reserve(rowIndexes.size());
    for (const int row : rowIndexes)
    {
        QTableWidgetItem *numberItem = m_previewTable->item(row, 0);
        if (numberItem == nullptr)
        {
            continue;
        }

        bool indexOk = false;
        const int competitorIndex = numberItem->data(Qt::UserRole).toInt(&indexOk);
        if (indexOk)
        {
            competitorIndexes.push_back(competitorIndex);
        }
    }

    if (competitorIndexes.isEmpty())
    {
        QMessageBox::information(this, "Entries Preview", "Select one or more rows to delete.");
        return;
    }

    emit deleteCompetitorsRequested(competitorIndexes);
}

bool SetupTab::tryParseBirthYear(const QString &text, int *birthYear) const
{
    if (birthYear == nullptr)
    {
        return false;
    }

    if (text.trimmed().isEmpty())
    {
        *birthYear = 0;
        return true;
    }

    bool ok = false;
    const int parsed = text.trimmed().toInt(&ok);
    if (ok && parsed >= 1900 && parsed <= 2200)
    {
        *birthYear = parsed;
        return true;
    }

    return false;
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
    competitor.category = canonicalCategory(m_manualCategoryOverrideCombo->currentText());
    competitor.birthYear = m_manualBirthYearSpin->value();
    const QString autoCategory = categoryFromBirthYear(competitor.birthYear, m_eventYear);
    competitor.categoryOverride = !competitor.category.isEmpty() && competitor.category.compare(autoCategory, Qt::CaseInsensitive) != 0;
    competitor.affiliation = m_manualAffiliationEdit->text().trimmed();
    competitor.disciplineParticipation.insert(DisciplineType::Fencing, true);
    competitor.disciplineParticipation.insert(DisciplineType::Obstacle, true);
    competitor.disciplineParticipation.insert(DisciplineType::Swimming, true);
    competitor.disciplineParticipation.insert(DisciplineType::LaserRun, true);
    return competitor;
}

void SetupTab::clearManualCompetitorForm()
{
    m_manualBibEdit->clear();
    m_manualFirstNameEdit->clear();
    m_manualLastNameEdit->clear();
    m_manualGenderEdit->clear();
    m_manualCategoryOverrideCombo->setCurrentIndex(0);
    m_manualAffiliationEdit->clear();
    m_manualBirthYearSpin->setValue(QDate::currentDate().year());
}
