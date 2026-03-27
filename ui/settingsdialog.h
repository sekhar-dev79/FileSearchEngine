// ui/settingsdialog.h
//
// A modal dialog opened with Ctrl+, that lets the user change
// persistent settings without restarting the application.
//
// Layout: QFormLayout — label on the left, control on the right.
// The form is the standard Qt idiom for settings panels [web:313].
// Changes take effect only when the user clicks "Apply & Rescan".
// Clicking "Cancel" discards all changes.

#pragma once

#include <QDialog>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // Call before exec() to populate controls from current settings.
    void loadFromSettings();

    // Returns the scan root path the user typed or chose.
    [[nodiscard]] QString scanRoot() const;

    // Returns the index cap the user set.
    [[nodiscard]] int indexCap() const;

    // Returns whether the user wants to skip hidden files.
    [[nodiscard]] bool skipHidden() const;

private slots:
    void onBrowseClicked();

private:
    void setupUi();
    void applyStyles();

    QFormLayout *m_formLayout;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_btnLayout;
    QHBoxLayout *m_rootLayout;  // scan root row: line edit + browse button

    // ── Controls ──────────────────────────────────────────────
    QLineEdit   *m_rootEdit;       // scan root path
    QPushButton *m_browseBtn;      // "..." folder picker
    QSpinBox    *m_capSpinBox;     // index file count cap
    QCheckBox   *m_skipHiddenBox;  // skip hidden files

    // ── Dialog buttons ────────────────────────────────────────
    QPushButton *m_applyBtn;
    QPushButton *m_cancelBtn;
};
