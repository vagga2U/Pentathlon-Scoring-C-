#include "fencinggrouptab.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

#include <algorithm>

FencingGroupTab::FencingGroupTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);

    m_pistesSpin = new QSpinBox(this);
    m_pistesSpin->setRange(1, 20);
    m_pistesSpin->setValue(1);

    auto *assignButton = new QPushButton("Auto Assign Groups", this);
    toolbarLayout->addWidget(new QLabel("Number of pistes", this));
    toolbarLayout->addWidget(m_pistesSpin);
    toolbarLayout->addWidget(assignButton);
    toolbarLayout->addStretch();

    m_statusLabel = new QLabel("Enter number of pistes and click Auto Assign Groups.", this);
    m_statusLabel->setWordWrap(true);

    m_groupTree = new QTreeWidget(this);
    m_groupTree->setColumnCount(3);
    m_groupTree->setHeaderLabels({"Number", "Name", "Affiliation"});
    m_groupTree->header()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addWidget(toolbar);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_groupTree, 1);

    connect(assignButton, &QPushButton::clicked, this, [this]() {
        emit autoAssignGroupsRequested(m_pistesSpin->value());
    });
}

void FencingGroupTab::populate(const QVector<Competitor> &competitors)
{
    m_groupTree->clear();

    QMap<int, QVector<const Competitor *>> groups;
    QVector<const Competitor *> unassigned;
    for (const Competitor &competitor : competitors)
    {
        if (competitor.fencingGroup > 0)
        {
            groups[competitor.fencingGroup].push_back(&competitor);
        }
        else
        {
            unassigned.push_back(&competitor);
        }
    }

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

        const QString groupTitle = QString("Group %1 (%2)")
            .arg(groupLabelForIndex(it.key()))
            .arg(sorted.size() == 1 ? "1 fencer" : QString("%1 fencers").arg(sorted.size()));
        auto *groupItem = new QTreeWidgetItem(QStringList() << groupTitle);
        m_groupTree->addTopLevelItem(groupItem);

        for (const Competitor *competitor : sorted)
        {
            auto *item = new QTreeWidgetItem(groupItem);
            item->setText(0, competitor->bib);
            item->setText(1, competitor->displayName());
            item->setText(2, competitor->affiliation.trimmed().isEmpty() ? "No affiliation" : competitor->affiliation);
        }
    }

    if (!unassigned.isEmpty())
    {
        std::sort(unassigned.begin(), unassigned.end(), [](const Competitor *left, const Competitor *right) {
            const QString leftName = left->displayName().toLower();
            const QString rightName = right->displayName().toLower();
            if (leftName != rightName)
            {
                return leftName < rightName;
            }
            return left->bib.toLower() < right->bib.toLower();
        });

        auto *groupItem = new QTreeWidgetItem(QStringList() << QString("Unassigned (%1)").arg(unassigned.size()));
        m_groupTree->addTopLevelItem(groupItem);

        for (const Competitor *competitor : unassigned)
        {
            auto *item = new QTreeWidgetItem(groupItem);
            item->setText(0, competitor->bib);
            item->setText(1, competitor->displayName());
            item->setText(2, competitor->affiliation.trimmed().isEmpty() ? "No affiliation" : competitor->affiliation);
        }
    }

    m_groupTree->expandAll();
}

void FencingGroupTab::setStatusMessage(const QString &message, bool isError)
{
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(isError ? "color: #ff6666;" : QString());
}

QString FencingGroupTab::groupLabelForIndex(int groupIndex)
{
    if (groupIndex <= 0)
    {
        return "?";
    }

    QString label;
    int value = groupIndex;
    while (value > 0)
    {
        const int remainder = (value - 1) % 26;
        label.prepend(QChar(static_cast<char>('A' + remainder)));
        value = (value - 1) / 26;
    }
    return label;
}
