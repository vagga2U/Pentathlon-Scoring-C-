#pragma once

#include "eventmodel.h"
#include "fencinggrouptab.h"

#include <QComboBox>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVector>
#include <QWidget>

class FencingResultsTab : public QWidget
{
    Q_OBJECT

public:
    explicit FencingResultsTab(QWidget *parent = nullptr);

    void setRoundSchedule(const QVector<FencingRoundScheduleRow> &scheduleRows);
    void setBonusRoundEnabled(bool enabled);
    void setPairLimit(int value);
    bool isBonusRoundEnabled() const;
    int pairLimit() const;
    void populate(const QVector<Competitor> &competitors);
    QVector<int> rankedCompetitorIndexes() const;

signals:
    void rawResultChanged(int competitorIndex, const QString &rawResult);
    void penaltyChanged(int competitorIndex, const QString &penalty);
    void recalculateRequested();
    void bonusRoundEnabledChanged(bool enabled);

private slots:
    void handleRoundCellChanged(int row, int column);
    void handleMatrixCellChanged(int row, int column);
    void handleViewModeChanged(int index);
    void handlePairLimitChanged(int value);

private:
    struct BoutRowInfo
    {
        int leftCompetitorIndex = -1;
        int rightCompetitorIndex = -1;
        QString key;
    };

    QMap<QString, int> parseFencingMatrixRow(const QString &raw) const;
    QString serializeFencingMatrixRow(const QMap<QString, int> &scoresByOpponentBib) const;
    int fencingMatrixScoreForPair(const Competitor &competitor, const QString &opponentBib) const;
    void populateRoundTable(const QVector<Competitor> &competitors);
    void populateMatrixTable(const QVector<Competitor> &competitors);
    void refreshMatrixHighlightsAndTotals();

    QComboBox *m_viewMode = nullptr;
    QPushButton *m_bonusRoundButton = nullptr;
    QSpinBox *m_pairLimitSpin = nullptr;
    QLabel *m_pairLimitStatusLabel = nullptr;
    QStackedWidget *m_views = nullptr;
    QTableWidget *m_roundTable = nullptr;
    QTableWidget *m_matrixTable = nullptr;
    QVector<FencingRoundScheduleRow> m_roundScheduleRows;
    QVector<int> m_rowToCompetitorIndex;
    QVector<int> m_matrixIndexToCompetitorIndex;
    QVector<BoutRowInfo> m_boutRows;
    QMap<QString, QString> m_boutPenaltyNotes;
    QVector<Competitor> m_lastCompetitors;
    QVector<int> m_rankedCompetitorIndexes;
    QSet<qulonglong> m_highlightedMismatchCells;
    int m_lastValidPairLimit = 1;
    bool m_updating = false;
};
