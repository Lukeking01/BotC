#pragma once
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include <unordered_map>
#include "storyteller.h" // for Character, StorytellerWindow::colors, etc.

class CharacterSelectionDialog : public QDialog {
    Q_OBJECT
public:
    CharacterSelectionDialog(const std::vector<Character> &characters,
                             int numPlayers,
                             QWidget *parent = nullptr);

    std::vector<Character> selectedCharacters() const;

private:
    QWidget *circleWidget;
    QLabel *countsLabel;
    std::vector<Character> allCharacters;
    std::unordered_map<QString,bool> selectedMap; // map by character ID
    int numPlayers;

    void setupCircle();
    void updateCounts();
};


