#pragma once
#include "CharacterSelectionDialog.h"
#include <QMessageBox>

class BluffSelectionDialog : public CharacterSelectionDialog {
    Q_OBJECT
public:
    explicit BluffSelectionDialog(const std::vector<Character>& characters, QWidget* parent = nullptr);

protected:
    void accept() override;
};
