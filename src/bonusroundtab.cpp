#include "bonusroundtab.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <QSet>

BonusRoundTab::BonusRoundTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Bonus Round Placement Entry (manual): enter final place and athlete.", this));
    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    auto *applyButton = new QPushButton("Apply Placings", this);
    auto *exportBracketButton = new QPushButton("Export Bracket Sheet (PDF)...", this);
    toolbarLayout->addWidget(applyButton);
    toolbarLayout->addWidget(exportBracketButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({"Place", "Athlete", "Score"});
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table, 1);

    connect(applyButton, &QPushButton::clicked, this, [this]() {
        applyPlacings();
    });
    connect(exportBracketButton, &QPushButton::clicked, this, [this]() {
        emit exportBracketSheetRequested();
    });
}

void BonusRoundTab::populate(const QVector<Competitor> &competitors, const QVector<int> &rankedCompetitorIndexes)
{
    Q_UNUSED(rankedCompetitorIndexes);

    m_competitors = competitors;
    m_rankedCompetitorIndexes = rankedCompetitorIndexes;
    m_fencingCompetitorIndexes.clear();
    m_athleteLabels.clear();
    m_competitorIndexByLabel.clear();
    m_competitorIndexesByDisplayNameHash.clear();

    for (int index = 0; index < m_competitors.size(); ++index)
    {
        if (!competitorParticipatesInDiscipline(m_competitors.at(index), DisciplineType::Fencing))
        {
            continue;
        }

        m_fencingCompetitorIndexes.push_back(index);
        const QString displayName = m_competitors.at(index).displayName().trimmed();
        const QString bib = m_competitors.at(index).bib.trimmed();
        const QString label = bib.isEmpty()
            ? displayName
            : QString("%1 (%2)").arg(displayName, bib);

        m_athleteLabels.push_back(label);
        m_competitorIndexByLabel.insert(label.toLower(), index);
        m_competitorIndexesByDisplayNameHash[qHash(displayName.toLower())].push_back(index);
    }

    applyStoredPlacingsToSelection();
    rebuildTable();
    updatePlacementsFromInputs(false);
}

void BonusRoundTab::setPlacingsByBib(const QMap<QString, int> &placingsByBib)
{
    m_placingsByBib = placingsByBib;
    applyStoredPlacingsToSelection();
    if (!m_competitors.isEmpty())
    {
        rebuildTable();
        updatePlacementsFromInputs(false, true);
    }
}

void BonusRoundTab::applyPlacings()
{
    updatePlacementsFromInputs(true, true);
}

QVector<int> BonusRoundTab::rankedCompetitorIndexes() const
{
    return m_rankedCompetitorIndexes;
}

void BonusRoundTab::applyStoredPlacingsToSelection()
{
    m_selectedCompetitorByPlace.clear();
    if (m_competitors.isEmpty() || m_placingsByBib.isEmpty())
    {
        return;
    }

    QSet<int> usedPlaces;
    for (int competitorIndex = 0; competitorIndex < m_competitors.size(); ++competitorIndex)
    {
        const Competitor &competitor = m_competitors.at(competitorIndex);
        if (!competitorParticipatesInDiscipline(competitor, DisciplineType::Fencing))
        {
            continue;
        }

        const QString bib = competitor.bib.trimmed();
        if (bib.isEmpty() || !m_placingsByBib.contains(bib))
        {
            continue;
        }

        const int place = m_placingsByBib.value(bib);
        if (place <= 0 || usedPlaces.contains(place))
        {
            continue;
        }

        usedPlaces.insert(place);
        m_selectedCompetitorByPlace.insert(place, competitorIndex);
    }
}

void BonusRoundTab::rebuildTable()
{
    m_updating = true;
    m_table->clearContents();
    m_table->setRowCount(0);

    const QMap<int, int> scoreByRank = loadBonusRoundScoringTable();
    const int rows = maxPlaceCount();
    for (int row = 0; row < rows; ++row)
    {
        const int place = row + 1;
        m_table->insertRow(row);

        auto *placeItem = new QTableWidgetItem(QString::number(place));
        placeItem->setFlags(placeItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 0, placeItem);

        auto *athleteCombo = new QComboBox(m_table);
        athleteCombo->setEditable(true);
        athleteCombo->setInsertPolicy(QComboBox::NoInsert);
        athleteCombo->setMaxVisibleItems(20);
        athleteCombo->addItems(m_athleteLabels);
        athleteCombo->setCurrentIndex(-1);

        const int selectedIndex = m_selectedCompetitorByPlace.value(place, -1);
        if (selectedIndex >= 0 && selectedIndex < m_competitors.size())
        {
            const QString selectedName = m_competitors.at(selectedIndex).displayName().trimmed();
            const QString selectedBib = m_competitors.at(selectedIndex).bib.trimmed();
            const QString selectedLabel = selectedBib.isEmpty()
                ? selectedName
                : QString("%1 (%2)").arg(selectedName, selectedBib);
            athleteCombo->setCurrentText(selectedLabel);
        }

        connect(athleteCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
            if (!m_updating)
            {
                updatePlacementsFromInputs(false);
            }
        });
        if (athleteCombo->lineEdit() != nullptr)
        {
            connect(athleteCombo->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
                if (!m_updating)
                {
                    updatePlacementsFromInputs(false);
                }
            });
        }
        m_table->setCellWidget(row, 1, athleteCombo);

        auto *scoreItem = new QTableWidgetItem(QString::number(scoreByRank.value(place, 0)));
        scoreItem->setFlags(scoreItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 2, scoreItem);
    }

    m_updating = false;
}

void BonusRoundTab::updatePlacementsFromInputs(bool emitSignal, bool forceEmit)
{
    QMap<int, int> placementsByCompetitorIndex;
    QSet<int> seenCompetitors;
    m_selectedCompetitorByPlace.clear();

    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        const int place = row + 1;
        auto *combo = qobject_cast<QComboBox *>(m_table->cellWidget(row, 1));
        if (combo == nullptr)
        {
            continue;
        }

        if (combo->lineEdit() != nullptr)
        {
            combo->lineEdit()->setStyleSheet(QString());
        }

        const QString enteredText = combo->currentText().trimmed();
        if (enteredText.isEmpty())
        {
            continue;
        }

        int competitorIndex = m_competitorIndexByLabel.value(enteredText.toLower(), -1);
        if (competitorIndex < 0)
        {
            const QString baseName = enteredText.section('(', 0, 0).trimmed().toLower();
            const QVector<int> matching = m_competitorIndexesByDisplayNameHash.value(qHash(baseName));
            if (matching.size() == 1)
            {
                competitorIndex = matching.front();
            }
        }

        if (competitorIndex < 0)
        {
            if (combo->lineEdit() != nullptr)
            {
                combo->lineEdit()->setStyleSheet("background-color:#f8d7da;");
            }
            continue;
        }

        if (seenCompetitors.contains(competitorIndex))
        {
            if (combo->lineEdit() != nullptr)
            {
                combo->lineEdit()->setStyleSheet("background-color:#fff3cd;");
            }
            continue;
        }

        seenCompetitors.insert(competitorIndex);
        placementsByCompetitorIndex.insert(competitorIndex, place);
        m_selectedCompetitorByPlace.insert(place, competitorIndex);
    }

    const bool changed = placementsByCompetitorIndex != m_lastPlacementsByCompetitorIndex;
    if (!changed && !forceEmit)
    {
        return;
    }

    m_lastPlacementsByCompetitorIndex = placementsByCompetitorIndex;
    if (emitSignal)
    {
        emit standingsChanged(placementsByCompetitorIndex);
    }
}

QMap<int, int> BonusRoundTab::loadBonusRoundScoringTable() const
{
    const QStringList candidatePaths = {
        QDir::current().absoluteFilePath("samples/fencingDE.csv"),
        QDir::current().absoluteFilePath("samples/FencingDE.csv"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../samples/fencingDE.csv"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../samples/FencingDE.csv"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../../samples/fencingDE.csv"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../../samples/FencingDE.csv")
    };

    QString tablePath;
    for (const QString &candidatePath : candidatePaths)
    {
        if (QFileInfo::exists(candidatePath))
        {
            tablePath = candidatePath;
            break;
        }
    }

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
    bool firstRow = true;
    while (!stream.atEnd())
    {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
        {
            continue;
        }

        if (firstRow)
        {
            firstRow = false;
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

int BonusRoundTab::maxPlaceCount() const
{
    return std::min(32, static_cast<int>(m_fencingCompetitorIndexes.size()));
}
