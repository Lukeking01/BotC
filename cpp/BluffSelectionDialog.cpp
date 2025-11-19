#include "BluffSelectionDialog.h"

BluffSelectionDialog::BluffSelectionDialog(const std::vector<Character>& characters, QWidget* parent)
    : CharacterSelectionDialog(characters, 3, parent)
{
    // The base constructor already builds the UI and calls setupCircle()
}

void BluffSelectionDialog::accept() {
    auto selected = selectedCharacters();
    if (selected.size() != 3) {
        QMessageBox::warning(this, "Invalid Selection", "You must select exactly 3 bluffs.");
        return;
    }
    QDialog::accept();
}
