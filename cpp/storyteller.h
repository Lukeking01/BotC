#pragma once
#include <QtWidgets>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

// ---------- Data models ----------
struct Character {
    QString id;
    QString name;
    QString team;
    QString ability;
    std::optional<int> first_night_order = 999;
    std::optional<int> other_night_order = 999;
    QString firstNightReminder;
    QString otherNightReminder;
    std::vector<QString> reminders;
};

struct Player {
    QString name;
    QString team;
    Character character;
    bool alive = true;
    bool poisoned = false;
    bool drunk = false;
    std::unordered_map<QString,bool> effects;

    QString status() const {
        QStringList flags;
        if (!alive) flags << "Dead";
        if (poisoned) flags << "Poisoned";
        if (drunk) flags << "Drunk";
        for (auto &kv : effects) if (kv.second) flags << kv.first;
        return flags.empty() ? "Healthy" : flags.join(" / ");
    }
};



// ---------- Main Window ----------
class StorytellerWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit StorytellerWindow(QWidget *parent = nullptr);
    
        // Static configs
    static const std::unordered_map<int, std::unordered_map<QString,int>> role_config;
    static const QMap<QString, QString> colors;
    static const std::vector<QString> all_teams;

private slots:
    void loadCharacterDBFromPath(const QString &path);
    void loadCharacterDB();
    void loadScript();
    void openAddPlayerDialog(Player* editPlayer = nullptr);

    
    
    void refreshPlayersCircle();
    void chooseBluffs();
    void showBluffs();
    void startNight();
    void runNightDialog(const std::vector<Player*>& ordered, int index);
    void endNight();
    void assignRandomCharacters();
    void generateGameDialog();
    void advanceDay();
    void sendMessageDialog();
    void applyEffect(Player* player);
    void setupMenu();
    void selectCharactersForRandomAssignment();
    void selectBluffsManually();

private:
    // UI members
    QTableWidget *playersTable;
    QLabel *headerLabel;
    QCheckBox *showAllCheckbox;
    QWidget *circleWidget = nullptr;
    QScrollArea* scrollArea = nullptr;
    QToolButton* menuButton;

    int day = 1;
    bool first_night = true;

    std::unordered_map<QString, Character> character_db;
    std::vector<Character> script_characters;
    std::vector<Player> players;
    std::vector<Character> bluffs;
    std::vector<QString> reminders;
    std::unordered_map<QString,bool> init_effects;
    std::vector<Character> selectedBluffs;


    

protected:
    void resizeEvent(QResizeEvent *event) override;
};
