#include "storyteller.h"
#include "CharacterSelectionDialog.h"
#include <QApplication>

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    StorytellerWindow w;
    w.show();

    return app.exec();
}
