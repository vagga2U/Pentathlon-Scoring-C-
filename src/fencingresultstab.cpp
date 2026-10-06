#include "fencingresultstab.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

FencingResultsTab::FencingResultsTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);

    m_viewMode = new QComboBox(this);
    m_viewMode->addItem("Round/Bout Entry");
    m_viewMode->addItem("Matrix Entry");

    m_pairLimitSpin = new QSpinBox(this);
    m_pairLimitSpin->setRange(1, 99);
    m_pairLimitSpin->setValue(1);
    m_pairLimitSpin->setKeyboardTracking(false);
    m_pairLimitSpin->setToolTip("B limit for mirrored pair validation (1-10).");
    m_bonusRoundButton = new QPushButton("Bonus Round", this);
    m_bonusRoundButton->setCheckable(true);
    m_bonusRoundButton->setChecked(false);
    m_bonusRoundButton->setToolTip("Enable to use the bonus-round direct elimination bracket.");
    m_pairLimitStatusLabel = new QLabel(this);
    m_pairLimitStatusLabel->setText(QString());
    m_pairLimitStatusLabel->setStyleSheet("color:#b00020;");

    auto *recalculateButton = new QPushButton("Calculate Scores", this);

    toolbarLayout->addWidget(new QLabel("View", this));
    toolbarLayout->addWidget(m_viewMode);
    toolbarLayout->addSpacing(12);
    toolbarLayout->addWidget(new QLabel("Pair limit (B)", this));
    toolbarLayout->addWidget(m_pairLimitSpin);
    toolbarLayout->addSpacing(12);
    toolbarLayout->addWidget(m_bonusRoundButton);
    toolbarLayout->addSpacing(12);
    toolbarLayout->addWidget(m_pairLimitStatusLabel);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(recalculateButton);

    m_views = new QStackedWidget(this);

    m_roundTable = new QTableWidget(this);
    m_roundTable->setColumnCount(8);
    m_roundTable->setHorizontalHeaderLabels({"Round", "Piste", "Fencer A", "Score", "Score", "Fencer B", "Groups", "Penalties"});
    m_roundTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_roundTable->setWordWrap(true);

    m_matrixTable = new QTableWidget(this);
    m_matrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_views->addWidget(m_roundTable);
    m_views->addWidget(m_matrixTable);

    layout->addWidget(toolbar);
    layout->addWidget(m_views, 1);

    connect(m_viewMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FencingResultsTab::handleViewModeChanged);
    connect(m_pairLimitSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &FencingResultsTab::handlePairLimitChanged);
    connect(m_bonusRoundButton, &QPushButton::toggled, this, [this](bool enabled) {
        emit bonusRoundEnabledChanged(enabled);
    });
    connect(recalculateButton, &QPushButton::clicked, this, [this]() {
        emit recalculateRequested();
    });
    connect(m_roundTable, &QTableWidget::cellChanged, this, &FencingResultsTab::handleRoundCellChanged);
    connect(m_matrixTable, &QTableWidget::cellChanged, this, &FencingResultsTab::handleMatrixCellChanged);
}

void FencingResultsTab::setRoundSchedule(const QVector<FencingRoundScheduleRow> &scheduleRows)
{
    m_roundScheduleRows = scheduleRows;
}

void FencingResultsTab::setBonusRoundEnabled(bool enabled)
{
    if (m_bonusRoundButton != nullptr)
    {
        m_bonusRoundButton->setChecked(enabled);
    }
}

void FencingResultsTab::setPairLimit(int value)
{
    if (m_pairLimitSpin == nullptr)
    {
        return;
    }

    const int clamped = std::clamp(value, 1, 10);
    m_updating = true;
    m_pairLimitSpin->setValue(clamped);
    m_lastValidPairLimit = clamped;
    if (m_pairLimitStatusLabel != nullptr)
    {
        m_pairLimitStatusLabel->clear();
    }
    m_updating = false;
}

bool FencingResultsTab::isBonusRoundEnabled() const
{
    return m_bonusRoundButton != nullptr && m_bonusRoundButton->isChecked();
}

int FencingResultsTab::pairLimit() const
{
    return m_pairLimitSpin != nullptr ? std::clamp(m_pairLimitSpin->value(), 1, 10) : 1;
}

QVector<int> FencingResultsTab::rankedCompetitorIndexes() const
{
    return m_rankedCompetitorIndexes;
}

void FencingResultsTab::populate(const QVector<Competitor> &competitors)
{
    m_updating = true;
    m_lastCompetitors = competitors;

    m_rowToCompetitorIndex.clear();
    for (int index = 0; index < competitors.size(); ++index)
    {
        if (competitorParticipatesInDiscipline(competitors.at(index), DisciplineType::Fencing))
        {
            m_rowToCompetitorIndex.push_back(index);
        }
    }

    populateRoundTable(competitors);
    populateMatrixTable(competitors);
    m_updating = false;
}

void FencingResultsTab::handleRoundCellChanged(int row, int column)
{
    if (m_updating)
    {
        return;
    }

    if (row < 0 || row >= m_boutRows.size())
    {
        return;
    }

    const int leftScoreColumn = 3;
    const int rightScoreColumn = 4;
    const int penaltyColumn = 7;
    const BoutRowInfo boutInfo = m_boutRows.at(row);
    if (boutInfo.leftCompetitorIndex < 0 || boutInfo.rightCompetitorIndex < 0)
    {
        return;
    }

    if (column == penaltyColumn)
    {
        QTableWidgetItem *item = m_roundTable->item(row, penaltyColumn);
        m_boutPenaltyNotes.insert(boutInfo.key, item != nullptr ? item->text().trimmed() : QString());
        return;
    }

    if (column != leftScoreColumn && column != rightScoreColumn)
    {
        return;
    }

    const QString leftBib = m_roundTable->item(row, 2) != nullptr ? m_roundTable->item(row, 2)->data(Qt::UserRole).toString() : QString();
    const QString rightBib = m_roundTable->item(row, 5) != nullptr ? m_roundTable->item(row, 5)->data(Qt::UserRole).toString() : QString();
    if (leftBib.isEmpty() || rightBib.isEmpty())
    {
        return;
    }

    auto parseScore = [this, row](int scoreColumn) {
        const QTableWidgetItem *item = m_roundTable->item(row, scoreColumn);
        if (item == nullptr)
        {
            return 0;
        }
        bool ok = false;
        const int value = item->text().trimmed().toInt(&ok);
        return ok ? std::max(0, value) : 0;
    };

    const int leftScore = parseScore(leftScoreColumn);
    const int rightScore = parseScore(rightScoreColumn);

    QMap<QString, int> leftRow = parseFencingMatrixRow(m_lastCompetitors.at(boutInfo.leftCompetitorIndex).rawResults.value(DisciplineType::Fencing));
    QMap<QString, int> rightRow = parseFencingMatrixRow(m_lastCompetitors.at(boutInfo.rightCompetitorIndex).rawResults.value(DisciplineType::Fencing));
    leftRow.insert(rightBib, leftScore);
    rightRow.insert(leftBib, rightScore);

    emit rawResultChanged(boutInfo.leftCompetitorIndex, serializeFencingMatrixRow(leftRow));
    emit rawResultChanged(boutInfo.rightCompetitorIndex, serializeFencingMatrixRow(rightRow));
}

void FencingResultsTab::handleMatrixCellChanged(int row, int column)
{
    if (m_updating)
    {
        return;
    }

    const int competitorCount = m_matrixIndexToCompetitorIndex.size();
    const int matrixStartColumn = 2;
    const int matrixEndColumnExclusive = matrixStartColumn + competitorCount;
    const int penaltiesColumn = matrixEndColumnExclusive + 3;

    if (row < 0 || row >= competitorCount)
    {
        return;
    }

    if (column == penaltiesColumn)
    {
        const int competitorIndex = m_matrixIndexToCompetitorIndex.at(row);
        QTableWidgetItem *item = m_matrixTable->item(row, penaltiesColumn);
        emit penaltyChanged(competitorIndex, item != nullptr ? item->text().trimmed() : QString());
        return;
    }

    if (column < matrixStartColumn || column >= matrixEndColumnExclusive)
    {
        return;
    }

    const int opponentMatrixIndex = column - matrixStartColumn;
    if (opponentMatrixIndex == row)
    {
        return;
    }

    const int competitorIndex = m_matrixIndexToCompetitorIndex.at(row);
    const QString opponentBib = m_matrixTable->item(opponentMatrixIndex, 0) != nullptr
        ? m_matrixTable->item(opponentMatrixIndex, 0)->text().trimmed()
        : QString();
    if (opponentBib.isEmpty())
    {
        return;
    }

    QTableWidgetItem *item = m_matrixTable->item(row, column);
    bool ok = false;
    const int score = item != nullptr ? item->text().trimmed().toInt(&ok) : 0;
    const int normalized = ok ? std::max(0, score) : 0;
    if (item != nullptr && item->text().trimmed() != QString::number(normalized))
    {
        m_updating = true;
        item->setText(QString::number(normalized));
        m_updating = false;
    }

    QMap<QString, int> rowMap = parseFencingMatrixRow(m_lastCompetitors.at(competitorIndex).rawResults.value(DisciplineType::Fencing));
    rowMap.insert(opponentBib, normalized);
    emit rawResultChanged(competitorIndex, serializeFencingMatrixRow(rowMap));
    refreshMatrixHighlightsAndTotals();
    emit recalculateRequested();
}

void FencingResultsTab::handleViewModeChanged(int index)
{
    m_views->setCurrentIndex(index);
    if (index == 1)
    {
        refreshMatrixHighlightsAndTotals();
    }
}

void FencingResultsTab::handlePairLimitChanged(int)
{
    if (m_updating)
    {
        return;
    }

    if (m_pairLimitSpin == nullptr)
    {
        return;
    }

    const int enteredValue = m_pairLimitSpin->value();
    if (enteredValue < 1 || enteredValue > 10)
    {
        m_updating = true;
        m_pairLimitSpin->setValue(m_lastValidPairLimit);
        m_updating = false;
        if (m_pairLimitStatusLabel != nullptr)
        {
            m_pairLimitStatusLabel->setText("Pair limit must be 1-10. Value not applied.");
        }
        return;
    }

    m_lastValidPairLimit = enteredValue;
    if (m_pairLimitStatusLabel != nullptr)
    {
        m_pairLimitStatusLabel->clear();
    }
    if (m_viewMode != nullptr && m_viewMode->currentIndex() == 1)
    {
        refreshMatrixHighlightsAndTotals();
    }
    emit recalculateRequested();
}

QMap<QString, int> FencingResultsTab::parseFencingMatrixRow(const QString &raw) const
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

QString FencingResultsTab::serializeFencingMatrixRow(const QMap<QString, int> &scoresByOpponentBib) const
{
    QJsonObject matrix;
    for (auto it = scoresByOpponentBib.begin(); it != scoresByOpponentBib.end(); ++it)
    {
        matrix.insert(it.key(), std::max(0, it.value()));
    }
    QJsonObject root;
    root.insert("matrix", matrix);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

int FencingResultsTab::fencingMatrixScoreForPair(const Competitor &competitor, const QString &opponentBib) const
{
    return parseFencingMatrixRow(competitor.rawResults.value(DisciplineType::Fencing)).value(opponentBib, 0);
}

void FencingResultsTab::populateRoundTable(const QVector<Competitor> &competitors)
{
    m_boutRows.clear();
    m_roundTable->clearContents();
    m_roundTable->setRowCount(0);

    QMap<int, QVector<const Competitor *>> groups;
    for (const int competitorIndex : m_rowToCompetitorIndex)
    {
        const Competitor &competitor = competitors.at(competitorIndex);
        if (competitor.fencingGroup > 0)
        {
            groups[competitor.fencingGroup].push_back(&competitor);
        }
    }

    QMap<QString, int> descriptorToCompetitorIndex;
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

        QString groupLabel;
        int value = it.key();
        while (value > 0)
        {
            const int remainder = (value - 1) % 26;
            groupLabel.prepend(QChar(static_cast<char>('A' + remainder)));
            value = (value - 1) / 26;
        }

        for (int memberIndex = 0; memberIndex < sorted.size(); ++memberIndex)
        {
            const QString descriptor = QString("%1%2").arg(groupLabel).arg(memberIndex + 1);
            const Competitor *competitor = sorted.at(memberIndex);
            for (int globalIndex = 0; globalIndex < competitors.size(); ++globalIndex)
            {
                if (&competitors.at(globalIndex) == competitor)
                {
                    descriptorToCompetitorIndex.insert(descriptor, globalIndex);
                    break;
                }
            }
        }
    }

    struct BoutRow
    {
        int round = 0;
        int piste = 0;
        QString leftDescriptor;
        QString rightDescriptor;
        QString groups;
    };

    QVector<BoutRow> rows;
    for (const FencingRoundScheduleRow &scheduleRow : m_roundScheduleRows)
    {
        const QStringList entries = scheduleRow.bouts.split(',', Qt::SkipEmptyParts);
        for (const QString &entry : entries)
        {
            const QStringList pair = entry.trimmed().split('-', Qt::KeepEmptyParts);
            if (pair.size() < 2)
            {
                continue;
            }
            rows.push_back(BoutRow{scheduleRow.round, scheduleRow.piste, pair.at(0).trimmed(), pair.at(1).trimmed(), scheduleRow.matchup});
        }
    }

    m_roundTable->setRowCount(rows.size());
    m_boutRows.resize(rows.size());

    for (int row = 0; row < rows.size(); ++row)
    {
        const BoutRow &bout = rows.at(row);
        const int leftIndex = descriptorToCompetitorIndex.value(bout.leftDescriptor, -1);
        const int rightIndex = descriptorToCompetitorIndex.value(bout.rightDescriptor, -1);
        const QString leftBib = leftIndex >= 0 ? competitors.at(leftIndex).bib : QString();
        const QString rightBib = rightIndex >= 0 ? competitors.at(rightIndex).bib : QString();
        const QString leftName = leftIndex >= 0 ? competitors.at(leftIndex).displayName() : bout.leftDescriptor;
        const QString rightName = rightIndex >= 0 ? competitors.at(rightIndex).displayName() : bout.rightDescriptor;
        const int leftScore = (leftIndex >= 0 && !rightBib.isEmpty()) ? fencingMatrixScoreForPair(competitors.at(leftIndex), rightBib) : 0;
        const int rightScore = (rightIndex >= 0 && !leftBib.isEmpty()) ? fencingMatrixScoreForPair(competitors.at(rightIndex), leftBib) : 0;
        const QString key = QString("%1|%2|%3-%4").arg(bout.round).arg(bout.piste).arg(bout.leftDescriptor, bout.rightDescriptor);

        auto *roundItem = new QTableWidgetItem(bout.round == 0 ? "0" : QString::number(bout.round));
        auto *pisteItem = new QTableWidgetItem(QString::number(bout.piste));
        auto *leftItem = new QTableWidgetItem(QString("%1 (%2)").arg(leftName, leftBib));
        auto *leftScoreItem = new QTableWidgetItem(QString::number(leftScore));
        auto *rightScoreItem = new QTableWidgetItem(QString::number(rightScore));
        auto *rightItem = new QTableWidgetItem(QString("%1 (%2)").arg(rightName, rightBib));
        auto *groupsItem = new QTableWidgetItem(bout.groups);
        auto *penaltyItem = new QTableWidgetItem(m_boutPenaltyNotes.value(key));

        leftItem->setData(Qt::UserRole, leftBib);
        rightItem->setData(Qt::UserRole, rightBib);
        roundItem->setFlags(roundItem->flags() & ~Qt::ItemIsEditable);
        pisteItem->setFlags(pisteItem->flags() & ~Qt::ItemIsEditable);
        leftItem->setFlags(leftItem->flags() & ~Qt::ItemIsEditable);
        rightItem->setFlags(rightItem->flags() & ~Qt::ItemIsEditable);
        groupsItem->setFlags(groupsItem->flags() & ~Qt::ItemIsEditable);

        m_roundTable->setItem(row, 0, roundItem);
        m_roundTable->setItem(row, 1, pisteItem);
        m_roundTable->setItem(row, 2, leftItem);
        m_roundTable->setItem(row, 3, leftScoreItem);
        m_roundTable->setItem(row, 4, rightScoreItem);
        m_roundTable->setItem(row, 5, rightItem);
        m_roundTable->setItem(row, 6, groupsItem);
        m_roundTable->setItem(row, 7, penaltyItem);

        m_boutRows[row] = BoutRowInfo{leftIndex, rightIndex, key};
    }
}

void FencingResultsTab::populateMatrixTable(const QVector<Competitor> &competitors)
{
    m_matrixIndexToCompetitorIndex = m_rowToCompetitorIndex;
    std::sort(m_matrixIndexToCompetitorIndex.begin(), m_matrixIndexToCompetitorIndex.end(), [&competitors](int leftIndex, int rightIndex) {
        const QString leftBib = competitors.at(leftIndex).bib.trimmed();
        const QString rightBib = competitors.at(rightIndex).bib.trimmed();

        bool leftOk = false;
        bool rightOk = false;
        const int leftNumeric = leftBib.toInt(&leftOk);
        const int rightNumeric = rightBib.toInt(&rightOk);
        if (leftOk != rightOk)
        {
            return leftOk;
        }
        if (leftOk && rightOk && leftNumeric != rightNumeric)
        {
            return leftNumeric < rightNumeric;
        }

        const QString leftSortBib = leftBib.toLower();
        const QString rightSortBib = rightBib.toLower();
        if (leftSortBib != rightSortBib)
        {
            return leftSortBib < rightSortBib;
        }

        return competitors.at(leftIndex).displayName().toLower() < competitors.at(rightIndex).displayName().toLower();
    });

    const int competitorCount = m_matrixIndexToCompetitorIndex.size();
    const int matrixStartColumn = 2;
    const int matrixEndColumnExclusive = matrixStartColumn + competitorCount;
    const int victoriesColumn = matrixEndColumnExclusive;
    const int rankColumn = matrixEndColumnExclusive + 1;
    const int scoreColumn = matrixEndColumnExclusive + 2;
    const int penaltiesColumn = matrixEndColumnExclusive + 3;

    m_matrixTable->clearContents();
    m_matrixTable->setRowCount(competitorCount);
    m_matrixTable->setColumnCount(matrixEndColumnExclusive + 4);
    m_highlightedMismatchCells.clear();

    QStringList headers;
    headers << "No." << "Name";
    for (const int competitorIndex : m_matrixIndexToCompetitorIndex)
    {
        headers << competitors.at(competitorIndex).bib;
    }
    headers << "Victories" << "Rank" << "Score" << "Penalties";
    m_matrixTable->setHorizontalHeaderLabels(headers);

    for (int row = 0; row < competitorCount; ++row)
    {
        const Competitor &competitor = competitors.at(m_matrixIndexToCompetitorIndex.at(row));
        m_matrixTable->setItem(row, 0, new QTableWidgetItem(competitor.bib));
        m_matrixTable->setItem(row, 1, new QTableWidgetItem(competitor.displayName()));

        const QMap<QString, int> scoreRow = parseFencingMatrixRow(competitor.rawResults.value(DisciplineType::Fencing));
        for (int opponent = 0; opponent < competitorCount; ++opponent)
        {
            QTableWidgetItem *item = nullptr;
            if (opponent == row)
            {
                item = new QTableWidgetItem("—");
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }
            else
            {
                const QString opponentBib = competitors.at(m_matrixIndexToCompetitorIndex.at(opponent)).bib;
                item = new QTableWidgetItem(QString::number(scoreRow.value(opponentBib, 0)));
            }
            m_matrixTable->setItem(row, matrixStartColumn + opponent, item);
        }

        auto *victoriesItem = new QTableWidgetItem("0");
        auto *rankItem = new QTableWidgetItem("-");
        auto *scoreItem = new QTableWidgetItem(disciplineScoreDisplayValue(competitor, DisciplineType::Fencing));
        auto *penaltiesItem = new QTableWidgetItem(competitor.penalties.value(DisciplineType::Fencing));
        victoriesItem->setFlags(victoriesItem->flags() & ~Qt::ItemIsEditable);
        rankItem->setFlags(rankItem->flags() & ~Qt::ItemIsEditable);
        scoreItem->setFlags(scoreItem->flags() & ~Qt::ItemIsEditable);

        m_matrixTable->setItem(row, victoriesColumn, victoriesItem);
        m_matrixTable->setItem(row, rankColumn, rankItem);
        m_matrixTable->setItem(row, scoreColumn, scoreItem);
        m_matrixTable->setItem(row, penaltiesColumn, penaltiesItem);
    }

    refreshMatrixHighlightsAndTotals();
}

void FencingResultsTab::refreshMatrixHighlightsAndTotals()
{
    const int competitorCount = m_matrixIndexToCompetitorIndex.size();
    if (competitorCount <= 0)
    {
        m_rankedCompetitorIndexes.clear();
        return;
    }

    const int matrixStartColumn = 2;
    const int matrixEndColumnExclusive = matrixStartColumn + competitorCount;
    const int victoriesColumn = matrixEndColumnExclusive;
    const int rankColumn = matrixEndColumnExclusive + 1;
    if (m_matrixTable->columnCount() <= rankColumn || m_matrixTable->rowCount() != competitorCount)
    {
        return;
    }
    const int pairLimit = m_pairLimitSpin->value();
    QVector<QVector<int>> matrixValues(competitorCount, QVector<int>(competitorCount, 0));
    for (int row = 0; row < competitorCount; ++row)
    {
        for (int opponent = 0; opponent < competitorCount; ++opponent)
        {
            if (row == opponent)
            {
                continue;
            }

            bool ok = false;
            const QTableWidgetItem *item = m_matrixTable->item(row, matrixStartColumn + opponent);
            const int value = item != nullptr ? item->text().trimmed().toInt(&ok) : 0;
            matrixValues[row][opponent] = ok ? std::max(0, value) : 0;
        }
    }

    QVector<int> victories(competitorCount, 0);
    for (int i = 0; i < competitorCount; ++i)
    {
        int wins = 0;
        for (int j = 0; j < competitorCount; ++j)
        {
            if (i == j)
            {
                continue;
            }

            wins += matrixValues[i][j];
        }
        victories[i] = wins;
    }

    for (int row = 0; row < competitorCount; ++row)
    {
        if (QTableWidgetItem *item = m_matrixTable->item(row, victoriesColumn))
        {
            item->setText(QString::number(victories[row]));
        }
    }

    QVector<int> headToHeadWins(competitorCount, 0);
    QMap<int, QVector<int>> tiedGroups;
    for (int athlete = 0; athlete < competitorCount; ++athlete)
    {
        tiedGroups[victories[athlete]].push_back(athlete);
    }

    for (auto it = tiedGroups.begin(); it != tiedGroups.end(); ++it)
    {
        const QVector<int> tiedAthletes = it.value();
        if (tiedAthletes.size() <= 1)
        {
            continue;
        }

        for (const int athlete : tiedAthletes)
        {
            int winsWithinTie = 0;
            for (const int opponent : tiedAthletes)
            {
                if (athlete == opponent)
                {
                    continue;
                }

                const int athleteVsOpponent = matrixValues[athlete][opponent];
                winsWithinTie += athleteVsOpponent;
            }
            headToHeadWins[athlete] = winsWithinTie;
        }
    }

    QVector<int> rankingOrder;
    rankingOrder.reserve(competitorCount);
    for (int index = 0; index < competitorCount; ++index)
    {
        rankingOrder.push_back(index);
    }

    std::sort(rankingOrder.begin(), rankingOrder.end(), [&victories, &headToHeadWins](int left, int right) {
        if (victories[left] != victories[right])
        {
            return victories[left] > victories[right];
        }
        if (headToHeadWins[left] != headToHeadWins[right])
        {
            return headToHeadWins[left] > headToHeadWins[right];
        }
        return left < right;
    });

    QVector<int> rankByAthlete(competitorCount, 0);
    m_rankedCompetitorIndexes.clear();
    m_rankedCompetitorIndexes.reserve(competitorCount);
    for (int order = 0; order < rankingOrder.size(); ++order)
    {
        const int rankedMatrixIndex = rankingOrder.at(order);
        rankByAthlete[rankedMatrixIndex] = order + 1;
        if (rankedMatrixIndex >= 0 && rankedMatrixIndex < m_matrixIndexToCompetitorIndex.size())
        {
            m_rankedCompetitorIndexes.push_back(m_matrixIndexToCompetitorIndex.at(rankedMatrixIndex));
        }
    }
    for (int athlete = 0; athlete < competitorCount; ++athlete)
    {
        if (QTableWidgetItem *item = m_matrixTable->item(athlete, rankColumn))
        {
            item->setText(QString::number(rankByAthlete[athlete]));
        }
    }

    if (m_viewMode == nullptr || m_viewMode->currentIndex() != 1)
    {
        m_highlightedMismatchCells.clear();
        return;
    }

    const auto mismatchCellKey = [](int row, int column) -> qulonglong {
        return (static_cast<qulonglong>(static_cast<quint32>(row)) << 32)
            | static_cast<quint32>(column);
    };

    QSet<qulonglong> mismatchCells;
    for (int i = 0; i < competitorCount; ++i)
    {
        for (int j = i + 1; j < competitorCount; ++j)
        {
            const int pairTotal = matrixValues[i][j] + matrixValues[j][i];
            if (pairTotal != pairLimit)
            {
                mismatchCells.insert(mismatchCellKey(i, matrixStartColumn + j));
                mismatchCells.insert(mismatchCellKey(j, matrixStartColumn + i));
            }
        }
    }

    const QSet<qulonglong> toClear = m_highlightedMismatchCells - mismatchCells;
    const QSet<qulonglong> toSet = mismatchCells - m_highlightedMismatchCells;

    m_matrixTable->setUpdatesEnabled(false);
    for (const qulonglong key : toClear)
    {
        const int row = static_cast<int>(key >> 32);
        const int column = static_cast<int>(key & 0xffffffffu);
        QTableWidgetItem *item = m_matrixTable->item(row, column);
        if (item == nullptr)
        {
            continue;
        }

        item->setForeground(QBrush());
    }

    for (const qulonglong key : toSet)
    {
        const int row = static_cast<int>(key >> 32);
        const int column = static_cast<int>(key & 0xffffffffu);
        QTableWidgetItem *item = m_matrixTable->item(row, column);
        if (item == nullptr)
        {
            continue;
        }

        item->setForeground(QColor("#b00020"));
    }
    m_matrixTable->setUpdatesEnabled(true);

    m_highlightedMismatchCells = mismatchCells;
}
