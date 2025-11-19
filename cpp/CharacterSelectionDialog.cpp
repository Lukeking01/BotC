#include "CharacterSelectionDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>

CharacterSelectionDialog::CharacterSelectionDialog(const std::vector<Character> &characters,
                                                   int numPlayers,
                                                   QWidget *parent)
    : QDialog(parent), allCharacters(characters), numPlayers(numPlayers)
{
    setWindowTitle("Select Characters");
    resize(800, 800);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Scrollable area
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    circleWidget = new QWidget();
    scrollArea->setWidget(circleWidget);
    mainLayout->addWidget(scrollArea);

    countsLabel = new QLabel(this);
    countsLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(countsLabel);

    QDialogButtonBox *buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setupCircle();
    updateCounts();
}

std::vector<Character> CharacterSelectionDialog::selectedCharacters() const {
    std::vector<Character> result;
    for (auto &c : allCharacters) {
        if (selectedMap.count(c.id) && selectedMap.at(c.id))
            result.push_back(c);
    }
    return result;
}

void CharacterSelectionDialog::setupCircle() {
    int n = allCharacters.size();
    int buttonSize = 75;
    int columns = 9;

    // Sort characters by team
    QMap<QString, int> teamOrder = {
        {"townsfolk", 0},
        {"outsider", 1},
        {"minion", 2},
        {"demon", 3},
        {"evil townsfolk", 4}
    };

    std::vector<Character> sortedChars = allCharacters;
    std::sort(sortedChars.begin(), sortedChars.end(),
              [&](const Character &a, const Character &b) {
                  int orderA = teamOrder.value(a.team, 100);
                  int orderB = teamOrder.value(b.team, 100);
                  return orderA < orderB;
              });

    // Grid layout for scrollable area
    QGridLayout *grid = new QGridLayout(circleWidget);
    grid->setSpacing(10);
    grid->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    for (int i = 0; i < n; ++i) {
        const Character &c = sortedChars[i];
        QPushButton *btn =
            new QPushButton(c.name + "\n(" + c.team + ")", circleWidget);
        btn->setFixedSize(buttonSize, buttonSize);
        btn->setCheckable(true);
        btn->setStyleSheet(QString(
                               "QPushButton { border-radius:%1px; background:%2; color:white; font-size:12px; }"
                               "QPushButton:checked { border:3px solid gold; }")
                               .arg(buttonSize / 2)
                               .arg(StorytellerWindow::colors.value(c.team, "gray")));

        selectedMap[c.id] = false;

        connect(btn, &QPushButton::toggled, [this, c](bool checked) {
            selectedMap[c.id] = checked;
            updateCounts();
        });

        int row = i / columns;
        int col = i % columns;
        grid->addWidget(btn, row, col);
    }
}

void CharacterSelectionDialog::updateCounts() {
    std::unordered_map<QString, int> counts;
    for (auto &c : allCharacters)
        if (selectedMap[c.id])
            counts[c.team]++;

    QString selectedText = "Selected:\n";
    for (auto &team : StorytellerWindow::all_teams)
        selectedText += QString("%1: %2  ").arg(team).arg(counts[team]);

    QString recommendedText;
    int total = numPlayers;
    if (StorytellerWindow::role_config.count(total)) {
        recommendedText = "\nRecommended:\n";
        auto rec = StorytellerWindow::role_config.at(total);
        for (auto &team : StorytellerWindow::all_teams) {
            int value = 0;
            auto it = rec.find(team);
            if (it != rec.end())
                value = it->second;
            recommendedText += QString("%1: %2  ").arg(team).arg(value);
        }
    }

    countsLabel->setText(selectedText + recommendedText);
}
