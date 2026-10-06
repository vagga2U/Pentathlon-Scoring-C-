#pragma once

#include "eventmodel.h"

#include <QMap>
#include <QTableWidget>
#include <QVector>
#include <QWidget>

class QComboBox;

class BonusRoundTab : public QWidget
{
    Q_OBJECT

public:
    explicit BonusRoundTab(QWidget *parent = nullptr);

    void populate(const QVector<Competitor> &competitors, const QVector<int> &rankedCompetitorIndexes);
    void setPlacingsByBib(const QMap<QString, int> &placingsByBib);
    void applyPlacings();
    QVector<int> rankedCompetitorIndexes() const;

signals:
    void standingsChanged(const QMap<int, int> &placementsByCompetitorIndex);
    void exportBracketSheetRequested();

private:
    void applyStoredPlacingsToSelection();
    void rebuildTable();
    void updatePlacementsFromInputs(bool emitSignal, bool forceEmit = false);
    QMap<int, int> loadBonusRoundScoringTable() const;
    int maxPlaceCount() const;

    QTableWidget *m_table = nullptr;
    QVector<Competitor> m_competitors;
    QVector<int> m_fencingCompetitorIndexes;
    QVector<int> m_rankedCompetitorIndexes;
    QStringList m_athleteLabels;
    QMap<QString, int> m_competitorIndexByLabel;
    QMap<int, QVector<int>> m_competitorIndexesByDisplayNameHash;
    QMap<int, int> m_selectedCompetitorByPlace;
    QMap<int, int> m_lastPlacementsByCompetitorIndex;
    QMap<QString, int> m_placingsByBib;
    bool m_updating = false;
};
