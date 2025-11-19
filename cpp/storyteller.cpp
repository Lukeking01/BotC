#include "storyteller.h"
#include "CharacterSelectionDialog.h"
#include "BluffSelectionDialog.h"
#include <fstream>
#include <algorithm>
#include <random>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDialogButtonBox>

using json = nlohmann::json;

// ---------- Static config ----------
const std::unordered_map<int, std::unordered_map<QString,int>> StorytellerWindow::role_config = {
    {5,  {{"townsfolk",3},{"outsider",0},{"minion",1},{"demon",1}}},
    {6,  {{"townsfolk",3},{"outsider",1},{"minion",1},{"demon",1}}},
    {7,  {{"townsfolk",5},{"outsider",0},{"minion",1},{"demon",1}}},
    {8,  {{"townsfolk",5},{"outsider",1},{"minion",1},{"demon",1}}},
    {9,  {{"townsfolk",5},{"outsider",2},{"minion",1},{"demon",1}}},
    {10, {{"townsfolk",7},{"outsider",0},{"minion",2},{"demon",1}}},
    {11, {{"townsfolk",7},{"outsider",1},{"minion",2},{"demon",1}}},
    {12, {{"townsfolk",7},{"outsider",2},{"minion",2},{"demon",1}}},
    {13, {{"townsfolk",9},{"outsider",0},{"minion",3},{"demon",1}}},
    {14, {{"townsfolk",9},{"outsider",1},{"minion",3},{"demon",1}}},
    {15, {{"townsfolk",9},{"outsider",2},{"minion",3},{"demon",1}}}
};

const QMap<QString, QString> StorytellerWindow::colors = {
    {"townsfolk","blue"},
    {"demon","orange"},
    {"minion","red"},
    {"outsider","green"},
    {"evil townsfolk","red"}
};

const std::vector<QString> StorytellerWindow::all_teams = {"townsfolk","demon","minion","outsider","evil townsfolk"};

// ---------- Random helpers ----------
static std::mt19937& rng() {
    static std::random_device rd;
    static std::mt19937 mt(rd());
    return mt;
}

template<typename T>
static std::vector<T> sample(const std::vector<T>& data, size_t k) {
    std::vector<T> pool = data;
    if (k >= pool.size()) return pool;
    std::shuffle(pool.begin(), pool.end(), rng());
    pool.resize(k);
    return pool;
}

// ---------- Constructor ----------
StorytellerWindow::StorytellerWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Blood on the Clocktower - Qt");
    resize(1200, 800);

    setupMenu();

    // Create central container
    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setCentralWidget(central);

    // Header label
    headerLabel = new QLabel(QString("Day %1").arg(day), this);
    headerLabel->setAlignment(Qt::AlignCenter);
    headerLabel->setStyleSheet("font-weight:bold; font-size:18px; color:white; background: black;");
    headerLabel->setFixedHeight(40);
    layout->addWidget(headerLabel);

    // Scroll area (for player circle)
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumSize(800, 800);
    layout->addWidget(scrollArea, 1);

    // Background
    QPixmap bg("../../images/bkg.png");
    bg = bg.scaled(scrollArea->size(), Qt::IgnoreAspectRatio);

    QPalette p = scrollArea->palette();
    p.setBrush(QPalette::Window, bg);
    scrollArea->setPalette(p);

    // Checkbox to show all players (used in startNight)
    showAllCheckbox = new QCheckBox("Show All Players", this);
    showAllCheckbox->setChecked(true);
    layout->addWidget(showAllCheckbox);

    // Initialize data
    day = 1;
    first_night = true;
    init_effects.clear();

    loadCharacterDBFromPath("../../Master_BotC.json");
    loadScript();             // loads script_characters
    refreshPlayersCircle();   // draw players
}


// ---------- Slots implementation ----------

void StorytellerWindow::loadCharacterDBFromPath(const QString &path) {
    std::ifstream f(path.toStdString());
    if (!f) {
        QMessageBox::warning(this, "Error", "Cannot open Master_BotC.json at startup.");
        return;
    }

    json j;
    try { f >> j; } 
    catch(...) { 
        QMessageBox::warning(this, "Error", "Invalid JSON in Master_BotC.json."); 
        return; 
    }

    character_db.clear();
    for (auto &it : j) {
        Character c;
        c.id = QString::fromStdString(it.value("id",""));
        c.name = QString::fromStdString(it.value("name",""));
        c.team = QString::fromStdString(it.value("team",""));
        c.ability = QString::fromStdString(it.value("ability",""));
        
        // Allow night orders to be null
        if (it.contains("first_night_order") && !it["first_night_order"].is_null())
            c.first_night_order = it["first_night_order"];
        else
            c.first_night_order = -1; // -1 means no night action

        if (it.contains("other_night_order") && !it["other_night_order"].is_null())
            c.other_night_order = it["other_night_order"];
        else
            c.other_night_order = -1;

        c.firstNightReminder = QString::fromStdString(it.value("firstNightReminder",""));
        c.otherNightReminder = QString::fromStdString(it.value("otherNightReminder",""));
        if (it.contains("reminders")) {
            for (auto &r : it["reminders"])
                c.reminders.push_back(QString::fromStdString(r));
        }

        character_db[c.id] = c;
    }

    qDebug() << "Loaded" << character_db.size() << "characters at startup.";
}


void StorytellerWindow::loadCharacterDB() {
    QString path = QFileDialog::getOpenFileName(this, "Open Master_BotC.json", QDir::currentPath(), "JSON Files (*.json)");
    if (path.isEmpty()) return;
    std::ifstream f(path.toStdString());
    if (!f) { QMessageBox::warning(this,"Error","Cannot open file."); return; }

    json j;
    try { f >> j; } catch(...) { QMessageBox::warning(this,"Error","Invalid JSON."); return; }

    character_db.clear();
    for (auto &it : j) {
        Character c;
        c.id = QString::fromStdString(it.value("id",""));
        c.name = QString::fromStdString(it.value("name",""));
        c.team = QString::fromStdString(it.value("team",""));
        c.ability = QString::fromStdString(it.value("ability",""));
        // Handle optional night orders
        if (it.contains("first_night_order") && !it["first_night_order"].is_null())
            c.first_night_order = it["first_night_order"].get<int>();
        else
            c.first_night_order = std::nullopt;

        if (it.contains("other_night_order") && !it["other_night_order"].is_null())
            c.other_night_order = it["other_night_order"].get<int>();
        else
            c.other_night_order = std::nullopt;
        c.firstNightReminder = QString::fromStdString(it.value("firstNightReminder",""));
        c.otherNightReminder = QString::fromStdString(it.value("otherNightReminder",""));
        if (it.contains("reminders")) {
            for (auto &r : it["reminders"]) c.reminders.push_back(QString::fromStdString(r));
        }
        character_db[c.id] = c;
    }

    QMessageBox::information(this,"Loaded", QString("Loaded %1 characters").arg(character_db.size()));
}

void StorytellerWindow::loadScript() {
    QString path = QFileDialog::getOpenFileName(this, "Open script.json", "../../scripts", "JSON Files (*.json)");
    if (path.isEmpty()) return;

    std::ifstream f(path.toStdString());
    if (!f) { QMessageBox::warning(this,"Error","Cannot open script file."); return; }

    json j;
    try { f >> j; } catch(...) { QMessageBox::warning(this,"Error","Invalid script JSON."); return; }

    script_characters.clear();
    reminders.clear();
    init_effects.clear();

    for (auto &entry : j) {
        QString id = QString::fromStdString(entry.value("id",""));
        if (character_db.count(id)) {
            Character c = character_db.at(id);
            script_characters.push_back(c);
            for (auto &r : c.reminders) if (std::find(reminders.begin(), reminders.end(), r) == reminders.end()) reminders.push_back(r);
        }
    }

    std::sort(reminders.begin(), reminders.end());
    for (auto &r : reminders) init_effects[r] = false;
    QMessageBox::information(this,"Script Loaded", QString("Loaded %1 script characters and %2 reminders").arg(script_characters.size()).arg(reminders.size()));

    refreshPlayersCircle();
    //refreshPlayersTable();
}

// ---------- openAddPlayerDialog ----------
void StorytellerWindow::openAddPlayerDialog(Player* editPlayer) {
    QDialog dlg(this);
    dlg.setWindowTitle(editPlayer ? "Edit Player" : "Add Player");
    QFormLayout form(&dlg);

    QLineEdit *nameEdit = new QLineEdit(&dlg);
    if (editPlayer) nameEdit->setText(editPlayer->name);
    form.addRow("Player name:", nameEdit);

    // available characters
    std::vector<Character> available;
    if (editPlayer) {
        available = script_characters;
    } else {
        std::unordered_set<QString> assigned;
        for (auto &p : players) assigned.insert(p.character.id);
        for (auto &c : script_characters) if (!assigned.count(c.id)) available.push_back(c);
    }

    QComboBox *charBox = new QComboBox(&dlg);
    for (auto &c : available) charBox->addItem(c.name);
    if (editPlayer) {
        int idx = charBox->findText(editPlayer->character.name);
        if (idx>=0) charBox->setCurrentIndex(idx);
    }
    form.addRow("Character:", charBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) return;

        QString selectedName = charBox->currentText();
        auto it = std::find_if(script_characters.begin(), script_characters.end(), [&](const Character &c){ return c.name == selectedName; });
        if (it == script_characters.end()) return;

        if (editPlayer) {
            editPlayer->name = name;
            editPlayer->character = *it;
            editPlayer->team = it->team;
        } else {
            Player p;
            p.name = name;
            p.character = *it;
            p.team = it->team;
            p.effects = init_effects;
            players.push_back(p);
        }

        //refreshPlayersTable();
        refreshPlayersCircle();
    }
}

// ---------- refreshPlayersTable ----------
void StorytellerWindow::refreshPlayersCircle() {
    if (!scrollArea) return;

    int n = players.size();
    if (n == 0) return;

    // --- Recreate the circle widget safely ---
    if (circleWidget) {
        scrollArea->takeWidget();
        delete circleWidget;
        circleWidget = nullptr;
    }

    circleWidget = new QWidget();
    //circleWidget->setStyleSheet("background:#222;");
    scrollArea->setWidget(circleWidget);
    scrollArea->setWidgetResizable(true);
    

    // --- Compute center and radius dynamically ---
    QSize areaSize = scrollArea->viewport()->size();
    int width = areaSize.width();
    int height = areaSize.height();
    QPoint center(width / 2, height / 2);

    // Radius based on number of players and window size
    int maxRadius = std::min(width, height) / 2 - 80;
    int radius = std::clamp(150 + (n - 1) * 20, 150, maxRadius);

    // --- Color map for effects ---
    QMap<QString, QString> effectColors = {
        {"Poisoned", "purple"},
        {"Stunned", "orange"},
        {"Protected", "green"},
        {"Bluffed", "yellow"}
    };

    // --- Draw players around circle ---
    for (int i = 0; i < n; ++i) {
        Player* p = &players[i];

        int buttonSize = 90; // both width and height the same
        double angle = 2 * M_PI * i / n;
        int x = center.x() + radius * std::cos(angle) - buttonSize / 2;
        int y = center.y() + radius * std::sin(angle) - buttonSize / 2;

        // Player button
        // btn->setStyleSheet(QString("background:%1; color:white; font-weight:bold;").arg(bg));
        
        QPushButton *btn = new QPushButton("", circleWidget);
        btn->setGeometry(x, y, buttonSize, buttonSize);

        // Load image
        QString imgPath = QString("../../Resources/botc_icons/%1.png")
                            .arg(p->character.name);

        btn->setIcon(QIcon(imgPath));
        btn->setIconSize(QSize(buttonSize, buttonSize));   // fills entire circle

        // Circular button styling
        btn->setStyleSheet(QString(
            "QPushButton {"
            "    border-radius:%1px;"
            "    border:2px solid white;"
            "    color: white;"
            "    background-color:gray;" 
            "    padding:0px;"            // must be 0 so the image fits perfectly
            "}"
            "QPushButton:hover {"
            "    border:3px solid gold;"
            "}"
        ).arg(buttonSize / 2));

        btn->show();

        QLabel* nameLbl = new QLabel(p->name + "\n" + p->character.name, circleWidget);
        nameLbl->setAlignment(Qt::AlignCenter);
        nameLbl->setGeometry(x, y + buttonSize + 2, buttonSize, 30);
        nameLbl->setStyleSheet(
            "color:white;"
            "font-weight:bold;"
            "font-size:14px;"
        );
        nameLbl->show();



        // Tooltip
        btn->setToolTip(QString("First Night: %1\nOther Night: %2")
            .arg(p->character.firstNightReminder.isEmpty() ? "(none)" : p->character.firstNightReminder)
            .arg(p->character.otherNightReminder.isEmpty() ? "(none)" : p->character.otherNightReminder));

        // Popup menu
        int idx = i;
        connect(btn, &QPushButton::clicked, [this, idx, btn, effectColors]() {
            QMenu menu;

            QAction *editAction = menu.addAction("Edit Player");
            connect(editAction, &QAction::triggered, [this, idx]() { openAddPlayerDialog(&players[idx]); });

            QAction *killAction = menu.addAction(players[idx].effects["Dead"] ? "Revive" : "Kill");
            connect(killAction, &QAction::triggered, [this, idx]() {
                players[idx].effects["Dead"] = !players[idx].effects["Dead"];
                refreshPlayersCircle();
            });

            QAction *poisonAction = menu.addAction(players[idx].effects["Poisoned"] ? "Remove Poison" : "Poison");
            connect(poisonAction, &QAction::triggered, [this, idx]() {
                players[idx].effects["Poisoned"] = !players[idx].effects["Poisoned"];
                refreshPlayersCircle();
            });

            

            QAction *effectAction = menu.addAction("Apply Effect");
            connect(effectAction, &QAction::triggered, [this, idx]() { applyEffect(&players[idx]); });

            menu.exec(btn->mapToGlobal(QPoint(0, btn->height())));
        });

        // Status label
        // QLabel *statusLbl = new QLabel(players[i].status(), circleWidget);
        // statusLbl->setGeometry(x, y + 60, 80, 20);
        // statusLbl->setAlignment(Qt::AlignCenter);
        // statusLbl->setStyleSheet("color:white; font-size:12px;");
        // statusLbl->show();
        // Effect/status circles (closer to center)
        // === Effects and statuses (draw line inward) ===
        // === Effects and statuses (aligned correctly toward center) ===
        int effectCircleSize = 45; // diameter of inner effect circles
        int effectSpacing = 45;    // distance between each effect circle toward center
        

        // Player center point (not top-left)
        int px = center.x() + radius * std::cos(angle);
        int py = center.y() + radius * std::sin(angle);

        int numEffects = 0;
        for (auto &kv : players[i].effects)
            if (kv.second) numEffects++;

        // === Status circle (just inside last effect circle) ===
        QString statusColor = players[i].effects["Dead"] ? "red" : "limegreen";
        int sx = center.x() + (0.78*radius - (1) * effectSpacing) * std::cos(angle) - effectCircleSize / 2;
        int sy = center.y() + (0.78*radius - (1) * effectSpacing) * std::sin(angle) - effectCircleSize / 2;

        QPushButton *statusCircle = new QPushButton(players[i].effects["Dead"] ? "Dead" : "Alive", circleWidget);
        statusCircle->setGeometry(sx, sy, effectCircleSize, effectCircleSize);
        
        statusCircle->setStyleSheet(QString(
                    "background:%1; border-radius:%2px; border:2px solid white; "
                    "color:white; font-size:10px; font-weight:bold; padding:2px;"
                ).arg(statusColor).arg(effectCircleSize / 2));
        statusCircle->setToolTip(players[i].status());
        connect(statusCircle, &QPushButton::clicked, [this, i]() mutable {
                    players[i].effects["Dead"] = !players[i].effects["Dead"];
                    refreshPlayersCircle();
                });
        statusCircle->show();
        
        // For each active effect, draw button circles going inward
        int effectIndex = 0;
        for (auto &kv : players[i].effects) {
            
            if (!kv.second) continue;
            if (kv.first != "Dead") {
                int ex = center.x() + (0.78*radius - (effectIndex + 2) * effectSpacing) * std::cos(angle) - effectCircleSize / 2;
                int ey = center.y() + (0.78*radius - (effectIndex + 2) * effectSpacing) * std::sin(angle) - effectCircleSize / 2;


                QString color = effectColors.contains(kv.first) ? effectColors[kv.first] : "black";

                QPushButton *effectBtn = new QPushButton(kv.first, circleWidget);
                effectBtn->setGeometry(ex, ey, effectCircleSize, effectCircleSize);
                effectBtn->setToolTip(QString("Click to remove '%1'").arg(kv.first));
                effectBtn->setStyleSheet(QString(
                    "background:%1; border-radius:%2px; border:2px solid white; "
                    "color:white; font-size:9px; font-weight:bold; padding:2px;"
                ).arg(color).arg(effectCircleSize / 2));

                connect(effectBtn, &QPushButton::clicked, [this, i, kv]() mutable {
                    players[i].effects[kv.first] = false;
                    refreshPlayersCircle();
                });

                effectBtn->show();
                effectIndex++;
            }
        }

        


    }

    headerLabel->setText(QString("Day %1").arg(day));
    // Background
    QPixmap bg("../../images/bkg1.png");
    bg = bg.scaled(circleWidget->size(), Qt::IgnoreAspectRatio);

    QPalette p = circleWidget->palette();
    p.setBrush(QPalette::Window, bg);
    circleWidget->setPalette(p);
    circleWidget->setAutoFillBackground(true);
}


void StorytellerWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    QPixmap bg("../../images/bkg.png");
    bg = bg.scaled(scrollArea->size(), Qt::IgnoreAspectRatio);

    QPalette p = scrollArea->palette();
    p.setBrush(QPalette::Window, bg);
    scrollArea->setPalette(p);
    if (isVisible() && scrollArea) {
        QTimer::singleShot(10, this, [this]() { refreshPlayersCircle(); });
    }
}

// ---------- setup menu ----------
void StorytellerWindow::setupMenu() {
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, toolbar);

    // Add a spacer to push items to the right
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Menu button
    menuButton = new QToolButton(this);
    menuButton->setText("Game Menu");
    menuButton->setPopupMode(QToolButton::InstantPopup);
    menuButton->setFixedSize(120, 30);
    toolbar->addWidget(menuButton);

    // Create the menu
    QMenu *gameMenu = new QMenu(this);
    menuButton->setMenu(gameMenu);

    // Add actions with shortcuts
    QAction *advanceDay = new QAction("Advance Day", this);
    advanceDay->setShortcut(QKeySequence("Ctrl+D"));
    connect(advanceDay, &QAction::triggered, this, &StorytellerWindow::advanceDay);

    QAction *startNight = new QAction("Start Night", this);
    startNight->setShortcut(QKeySequence("Ctrl+N"));
    connect(startNight, &QAction::triggered, this, &StorytellerWindow::startNight);

    QAction *assignChars = new QAction("Assign Random Characters", this);
    connect(assignChars, &QAction::triggered, this, &StorytellerWindow::assignRandomCharacters);

    QAction *selectChars = new QAction("Select Characters", this);
    connect(selectChars, &QAction::triggered, this, &StorytellerWindow::selectCharactersForRandomAssignment);

    QAction *showBluffs = new QAction("Show Bluffs", this);
    connect(showBluffs, &QAction::triggered, this, &StorytellerWindow::showBluffs);

    QAction *generateGame = new QAction("Generate Game", this);
    connect(generateGame, &QAction::triggered, this, &StorytellerWindow::generateGameDialog);

    QAction *addPlayer = new QAction("Add Player", this);
    connect(addPlayer, &QAction::triggered, this, [this]() { openAddPlayerDialog(nullptr); });

    QAction *loadScript = new QAction("Load Script", this);
    connect(loadScript, &QAction::triggered, this, &StorytellerWindow::loadScript);

    QAction *selectBluffs = new QAction("Select Bluffs", this);
    connect(selectBluffs, &QAction::triggered, this, &StorytellerWindow::selectBluffsManually);


    // Add them to the popup menu
    gameMenu->addAction(advanceDay);
    gameMenu->addAction(startNight);
    gameMenu->addAction(assignChars);
    gameMenu->addAction(selectChars);
    gameMenu->addAction(showBluffs);
    gameMenu->addAction(generateGame);
    gameMenu->addAction(addPlayer);
    gameMenu->addAction(loadScript);
    gameMenu->addAction(selectBluffs);

    // 🔑 Add global shortcut support
    addAction(advanceDay);
    addAction(startNight);
    addAction(assignChars);
    addAction(selectChars);
    addAction(showBluffs);
    addAction(generateGame);
    addAction(addPlayer);
    addAction(loadScript);
    addAction(selectBluffs);

}


// ---------- chooseBluffs ----------
void StorytellerWindow::chooseBluffs() {
    std::unordered_set<QString> assigned;
    for (auto &p : players) assigned.insert(p.character.id);

    std::vector<Character> available;
    for (auto &c : script_characters) {
        if (!assigned.count(c.id) && (c.team == "townsfolk" || c.team == "outsider")) available.push_back(c);
    }

    bluffs = sample(available, std::min<size_t>(3, available.size()));
}

void StorytellerWindow::showBluffs() {
    QDialog dlg(this);
    dlg.setWindowTitle("Bluff Characters");
    dlg.resize(1200, 800);
    QVBoxLayout *v = new QVBoxLayout(&dlg);

    QLabel *title = new QLabel("Demon Bluffs", &dlg);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:48px; font-weight:bold;");
    v->addWidget(title);

    for (auto &c : bluffs) {
        QLabel *n = new QLabel(QString("%1 (%2)").arg(c.name, c.team), &dlg);
        n->setStyleSheet(QString("font-size:38px; color:%1;").arg(colors.value(c.team, "white")));
        QLabel *a = new QLabel(c.ability, &dlg);
        a->setWordWrap(true);
        v->addWidget(n);
        v->addWidget(a);
    }

    QPushButton *close = new QPushButton("Close", &dlg);
    v->addWidget(close);
    connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();
    refreshPlayersCircle();
    //refreshPlayersTable();
}

// ---------- startNight ----------
void StorytellerWindow::startNight() {
    std::vector<Player*> night_players;

    // Choose players based on first/other night reminders
    for (auto &p : players) {
        if (showAllCheckbox && showAllCheckbox->isChecked()) {
            night_players.push_back(&p);
        } else {
            if (first_night && !p.character.firstNightReminder.isEmpty()) night_players.push_back(&p);
            else if (!first_night && !p.character.otherNightReminder.isEmpty()) night_players.push_back(&p);
        }
    }

    if (night_players.empty()) {
        QMessageBox::information(this, "Night Phase", "No players with night actions.");
        return;
    }

    // Sort by night order safely using optional values
    std::sort(night_players.begin(), night_players.end(),
              [this](Player* a, Player* b) {
        int orderA = first_night ? (a->character.first_night_order.value_or(1000))
                                 : (a->character.other_night_order.value_or(1000));
        int orderB = first_night ? (b->character.first_night_order.value_or(1000))
                                 : (b->character.other_night_order.value_or(1000));
        return orderA < orderB;
    });

    runNightDialog(night_players, 0);
}

// ---------- runNightDialog ----------
void StorytellerWindow::runNightDialog(const std::vector<Player*> &ordered, int index) {
    if (index >= static_cast<int>(ordered.size())) {
        endNight();
        return;
    }

    Player* p = ordered[index];
    if (!p) {
        runNightDialog(ordered, index + 1);
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Night Action: " + p->name);
    dlg.resize(600, 700);
    QVBoxLayout *v = new QVBoxLayout(&dlg);

    QLabel *info = new QLabel(QString("Player: %1 (%2)").arg(p->name, p->character.name) + " ; " + p->status());
    v->addWidget(info);

    if (!p->character.firstNightReminder.isEmpty()) {
        QLabel *firstRem = new QLabel("First night reminder: " + p->character.firstNightReminder);
        firstRem->setWordWrap(true);
        firstRem->setStyleSheet("color:gray;");
        v->addWidget(firstRem);
    }

    if (!p->character.otherNightReminder.isEmpty()) {
        QLabel *otherRem = new QLabel("Other night reminder: " + p->character.otherNightReminder);
        otherRem->setWordWrap(true);
        otherRem->setStyleSheet("color:gray;");
        v->addWidget(otherRem);
    }

    // Dropdown for effects
    QComboBox *reminderBox = new QComboBox(&dlg);
    reminderBox->addItem("Choose effect...");
    for (auto &r : reminders) reminderBox->addItem(r);
    v->addWidget(new QLabel("Select effect to apply:"));
    v->addWidget(reminderBox);

    // Target checkboxes
    std::vector<QCheckBox*> targetChecks;
    for (auto &other : players) {
        if (other.name == p->name) continue;  // skip self
        QCheckBox *cb = new QCheckBox(other.name + " (" + other.character.name + ")", &dlg);
        v->addWidget(cb);
        targetChecks.push_back(cb);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    v->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString chosenEffect = reminderBox->currentText();
        if (chosenEffect != "Choose effect...") {
            int targetIndex = 0;
            for (auto &other : players) {
                if (other.name == p->name) continue;
                if (targetChecks[targetIndex]->isChecked()) {
                    other.effects[chosenEffect] = true;
                }
                ++targetIndex;
            }
        }

        refreshPlayersCircle();
        runNightDialog(ordered, index + 1); // next player
    } else {
        endNight();
    }
}


// ---------- endNight ----------
void StorytellerWindow::endNight() {
    first_night = false;
    refreshPlayersCircle();
}

// ---------- assignRandomCharacters ----------
void StorytellerWindow::assignRandomCharacters() {
    int num_players = players.size();
    if (num_players == 0) { QMessageBox::warning(this,"No Players","Add players first."); return; }
    if (!role_config.count(num_players)) { QMessageBox::warning(this,"Unsupported","Only 5–15 players supported."); return; }

    auto config = role_config.at(num_players);
    std::unordered_map<QString,std::vector<Character>> team_pools;
    for (auto &c : script_characters) team_pools[c.team].push_back(c);

    std::vector<Character> selected_characters;
    try {
        for (auto &kv : config) {
            QString team = kv.first;
            int count = kv.second;
            if (team_pools[team].size() < (size_t)count) throw std::runtime_error("Not enough characters in script for team: " + team.toStdString());
            auto samp = sample(team_pools[team], count);
            selected_characters.insert(selected_characters.end(), samp.begin(), samp.end());
        }
    } catch (std::exception &e) {
        QMessageBox::critical(this,"Character Assignment Error", e.what());
        return;
    }

    std::shuffle(selected_characters.begin(), selected_characters.end(), rng());
    for (int i=0;i<num_players;i++) {
        players[i].character = selected_characters[i];
        players[i].team = selected_characters[i].team;
        players[i].alive = true;
        players[i].poisoned = false;
        players[i].drunk = false;
        players[i].effects = init_effects;
    }

    chooseBluffs();
    first_night = true;
    day = 1;
    headerLabel->setText(QString("Day %1").arg(day));
    refreshPlayersCircle();
    //refreshPlayersTable();
}

// ---------- generateGameDialog ----------
void StorytellerWindow::generateGameDialog() {
    bool ok = false;
    int num = QInputDialog::getInt(this, "Number of Players",
                                   "Enter number of players (5–15):", 7, 5, 15, 1, &ok);
    if (!ok) return;

    if (!role_config.count(num)) {
        QMessageBox::warning(this, "Invalid", "Unsupported number of players");
        return;
    }

    players.clear();

    auto cfg = role_config.at(num);
    std::unordered_map<QString, std::vector<Character>> team_pool;
    for (auto &c : script_characters) team_pool[c.team].push_back(c);

    // Check for enough characters
    for (auto &kv : cfg) {
        QString team = kv.first;
        int needed = kv.second;
        if (team_pool[team].size() < (size_t)needed) {
            QMessageBox::critical(this, "Not enough characters",
                                  QString("Script does not have enough unique %1 characters").arg(team));
            return;
        }
    }

    // Select characters
    std::vector<Character> selected;
    for (auto &kv : cfg) {
        QString team = kv.first;
        int count = kv.second;
        auto s = sample(team_pool[team], count);
        selected.insert(selected.end(), s.begin(), s.end());
    }

    std::shuffle(selected.begin(), selected.end(), rng());

    // Create players with initialized fields
    for (int i = 0; i < num; i++) {
        Player p;
        p.name = QString("Player %1").arg(i + 1);
        p.character = selected[i];
        p.team = selected[i].team;
        p.alive = true;
        p.poisoned = false;
        p.drunk = false;
        p.effects = init_effects;   // copy of initial effects map
        players.push_back(p);
    }

    chooseBluffs();
    first_night = true;
    day = 1;
    headerLabel->setText(QString("Day %1").arg(day));
    refreshPlayersCircle();
}


// ---------- advanceDay ----------
void StorytellerWindow::advanceDay() {
    day++;
    first_night = false;
    headerLabel->setText(QString("Day %1").arg(day));
    refreshPlayersCircle();
    //refreshPlayersTable();
}

// ---------- sendMessageDialog ----------
void StorytellerWindow::sendMessageDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("Send Message");
    QVBoxLayout *v = new QVBoxLayout(&dlg);

    QComboBox *msgType = new QComboBox(&dlg);
    msgType->addItems({"You are now","You were chosen by","You were seen as","You learn that","You feel like"});
    v->addWidget(new QLabel("Message type:"));
    v->addWidget(msgType);

    QComboBox *charBox = new QComboBox(&dlg);
    for (auto &c : script_characters) charBox->addItem(c.name);
    v->addWidget(new QLabel("Character:"));
    v->addWidget(charBox);

    QDialogButtonBox box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    v->addWidget(&box);
    connect(&box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(&box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString msg = msgType->currentText() + " " + charBox->currentText();

        QDialog msgdlg(this);
        msgdlg.setWindowTitle("Message");
        QVBoxLayout *mv = new QVBoxLayout(&msgdlg);
        QLabel *l1 = new QLabel(msgType->currentText(), &msgdlg); l1->setStyleSheet("font-size:28px;");
        QLabel *l2 = new QLabel(charBox->currentText(), &msgdlg); l2->setStyleSheet("font-size:28px;");
        mv->addWidget(l1); mv->addWidget(l2);
        QPushButton *close = new QPushButton("Close", &msgdlg);
        mv->addWidget(close);
        connect(close, &QPushButton::clicked, &msgdlg, &QDialog::accept);
        msgdlg.exec();
    }
}

// ---------- applyEffect ----------
void StorytellerWindow::applyEffect(Player* player) {

    if (!player) return;

    // Create the dialog
    QDialog dlg(this);
    dlg.setWindowTitle("Apply Effect");
    QVBoxLayout *v = new QVBoxLayout(&dlg);

    // Dropdown to choose an effect from the global list of reminders
    QComboBox *reminderBox = new QComboBox();
    reminderBox->addItem("Choose effect...");
    for (auto &r : reminders) reminderBox->addItem(r);

    v->addWidget(new QLabel("Select effect to apply:"));
    v->addWidget(reminderBox);

    // Ok/Cancel buttons
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    v->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // Execute dialog
    if (dlg.exec() == QDialog::Accepted) {
        QString chosenEffect = reminderBox->currentText();
        if (chosenEffect != "Choose effect...") {
            // Toggle the effect
            player->effects[chosenEffect] = !player->effects[chosenEffect];
        }
        refreshPlayersCircle();
        //refreshPlayersTable();
    }
}

void StorytellerWindow::selectCharactersForRandomAssignment() {
    CharacterSelectionDialog dlg(script_characters, players.size(), this);
    if (dlg.exec() == QDialog::Accepted) {
        auto selected = dlg.selectedCharacters();
        if (selected.size() < players.size()) {
            QMessageBox::warning(this, "Not enough characters", 
                                 "Select at least as many characters as players.");
            return;
        }
        std::shuffle(selected.begin(), selected.end(), rng());
        for (int i = 0; i < players.size(); ++i) {
            players[i].character = selected[i];
            players[i].team = selected[i].team;
            players[i].alive = true;
            players[i].effects = init_effects;
        }
        refreshPlayersCircle();
    }
}

void StorytellerWindow::selectBluffsManually() {
    // Filter only Townsfolk + Outsiders
    std::vector<Character> eligible;
    for (auto &c : script_characters) {
        if (c.team == "townsfolk" || c.team == "outsider")
            eligible.push_back(c);
    }

    BluffSelectionDialog dialog(eligible, this);
    if (dialog.exec() == QDialog::Accepted) {
        bluffs = dialog.selectedCharacters(); // store them in your class
        // QMessageBox::information(this, "Bluffs Selected",
        //                          QString("Selected bluffs:\n%1, %2, %3")
        //                          .arg(selectedBluffs[0].name)
        //                          .arg(selectedBluffs[1].name)
        //                          .arg(selectedBluffs[2].name));
    }
}

