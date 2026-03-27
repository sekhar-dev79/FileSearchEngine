// ui/settingsdialog.cpp
#include "settingsdialog.h"
#include "appSettings.h"

#include <QSettings>
#include <QFileDialog>
#include <QDir>
#include <QIcon>
#include <QFrame>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setModal(true);
    setMinimumWidth(420); // COMPACT: Slimmed down from 500

    setupUi();
    applyStyles();
    loadFromSettings();

    // Forces the dialog to perfectly wrap the internal contents
    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

// ── setupUi ───────────────────────────────────────────────────────────────────
void SettingsDialog::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    // COMPACT: Minimal outer margins
    m_mainLayout->setContentsMargins(12, 12, 12, 8);
    m_mainLayout->setSpacing(8);

    // ── The Settings Card (QFrame) ────────────────────────────
    auto *cardFrame = new QFrame(this);
    cardFrame->setObjectName(QStringLiteral("SettingsCard"));

    m_formLayout = new QFormLayout(cardFrame);
    // COMPACT: Minimal padding inside the card
    m_formLayout->setContentsMargins(12, 12, 12, 12);
    m_formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_formLayout->setHorizontalSpacing(8);
    // COMPACT: Tighter vertical spacing between rows
    m_formLayout->setVerticalSpacing(8);

    // ── Row 1: Scan root ──────────────────────────────────────
    m_rootLayout = new QHBoxLayout();
    m_rootLayout->setSpacing(4); // Tight gap between text box and button

    m_rootEdit = new QLineEdit(cardFrame);
    m_rootEdit->setObjectName(QStringLiteral("SettingsLineEdit"));
    m_rootEdit->setPlaceholderText(QStringLiteral("e.g. C:/Users/jayanth"));

    m_browseBtn = new QPushButton(QIcon(":/icons/folder.svg"), QStringLiteral(""), cardFrame);
    m_browseBtn->setObjectName(QStringLiteral("BrowseButton"));
    // COMPACT: Square 24px button matching the text box height
    m_browseBtn->setFixedSize(24, 24);
    m_browseBtn->setIconSize(QSize(14, 14));
    m_browseBtn->setToolTip(QStringLiteral("Browse for folder"));

    m_rootLayout->addWidget(m_rootEdit, 1);
    m_rootLayout->addWidget(m_browseBtn);

    m_formLayout->addRow(QStringLiteral("Scan root:"), m_rootLayout);

    // ── Row 2: Index cap ──────────────────────────────────────
    m_capSpinBox = new QSpinBox(cardFrame);
    m_capSpinBox->setObjectName(QStringLiteral("SettingsSpin"));
    m_capSpinBox->setRange(10'000, 5'000'000);
    m_capSpinBox->setSingleStep(10'000);
    m_capSpinBox->setSuffix(QStringLiteral(" files"));
    m_capSpinBox->setToolTip(
        QStringLiteral("Maximum number of files to index.\n"
                       "Lower values = faster scan, less complete index."));

    m_formLayout->addRow(QStringLiteral("Index cap:"), m_capSpinBox);

    // ── Row 3: Skip hidden files ──────────────────────────────
    m_skipHiddenBox = new QCheckBox(
        QStringLiteral("Skip hidden files and folders"), cardFrame);
    m_skipHiddenBox->setObjectName(QStringLiteral("SettingsCheck"));
    m_skipHiddenBox->setToolTip(
        QStringLiteral("Recommended: ON.\n"
                       "Hidden items include system files that can slow down scanning."));

    m_formLayout->addRow(QStringLiteral(""), m_skipHiddenBox);

    m_mainLayout->addWidget(cardFrame);

    // ── Dialog buttons ────────────────────────────────────────
    m_btnLayout  = new QHBoxLayout();
    m_btnLayout->setSpacing(6); // Tighter button spacing

    m_applyBtn   = new QPushButton(QStringLiteral("Apply & Rescan"), this);
    m_cancelBtn  = new QPushButton(QStringLiteral("Cancel"), this);

    m_applyBtn->setObjectName(QStringLiteral("ApplyButton"));
    m_cancelBtn->setObjectName(QStringLiteral("CancelButton"));

    m_applyBtn->setDefault(true);

    m_btnLayout->addStretch();
    m_btnLayout->addWidget(m_cancelBtn);
    m_btnLayout->addWidget(m_applyBtn);

    m_mainLayout->addLayout(m_btnLayout);

    // ── Connections ───────────────────────────────────────────
    connect(m_browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseClicked);
    connect(m_applyBtn,  &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

// ── loadFromSettings ──────────────────────────────────────────────────────────
void SettingsDialog::loadFromSettings()
{
    QSettings s;
    s.beginGroup(QLatin1StringView(AppSettings::GROUP_SCAN));
    m_rootEdit->setText(
        s.value(QLatin1StringView(AppSettings::KEY_SCAN_ROOT),
                AppSettings::defaultScanRoot()).toString());
    m_capSpinBox->setValue(
        s.value(QLatin1StringView(AppSettings::KEY_INDEX_CAP),
                AppSettings::defaultIndexCap()).toInt());
    m_skipHiddenBox->setChecked(
        s.value(QLatin1StringView(AppSettings::KEY_SKIP_HIDDEN),
                AppSettings::defaultSkipHidden()).toBool());
    s.endGroup();
}

QString SettingsDialog::scanRoot()  const { return m_rootEdit->text().trimmed(); }
int     SettingsDialog::indexCap()  const { return m_capSpinBox->value(); }
bool    SettingsDialog::skipHidden()const { return m_skipHiddenBox->isChecked(); }

// ── onBrowseClicked ───────────────────────────────────────────────────────────
void SettingsDialog::onBrowseClicked()
{
    const QString chosen = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select Scan Root Folder"),
        m_rootEdit->text().isEmpty() ? AppSettings::defaultScanRoot() : m_rootEdit->text());

    if (!chosen.isEmpty())
        m_rootEdit->setText(chosen);
}

// ── applyStyles ───────────────────────────────────────────────────────────────
void SettingsDialog::applyStyles()
{
    setStyleSheet(R"(
        /* Base Dialog */
        QDialog {
            background-color: #F8FAFC;
        }

        /* The White Form Card */
        QFrame#SettingsCard {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px; /* COMPACT: Sharp 2px corners */
        }

        /* Typography */
        QLabel {
            background: transparent;
            color: #475569;
            font-size: 11px; /* COMPACT: 11px global font */
            font-weight: 600;
        }

        /* Text Inputs (Slimmer) */
        QLineEdit#SettingsLineEdit, QSpinBox#SettingsSpin {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px;
            padding: 2px 6px;
            color: #0F172A;
            font-size: 11px;
            min-height: 20px; /* COMPACT: Ultra slim height */
        }
        QLineEdit#SettingsLineEdit:focus, QSpinBox#SettingsSpin:focus {
            border: 1px solid #3B82F6;
        }

        /* Spinbox Arrows */
        QSpinBox#SettingsSpin::up-button, QSpinBox#SettingsSpin::down-button {
            subcontrol-origin: border;
            width: 18px;
            background: #F8FAFC;
            border-left: 1px solid #CBD5E1;
        }
        QSpinBox#SettingsSpin::up-button {
            subcontrol-position: top right;
            border-bottom: 1px solid #CBD5E1;
        }
        QSpinBox#SettingsSpin::down-button {
            subcontrol-position: bottom right;
        }
        QSpinBox#SettingsSpin::up-button:hover, QSpinBox#SettingsSpin::down-button:hover {
            background-color: #E2E8F0;
        }
        QSpinBox#SettingsSpin::up-arrow { image: url(:/icons/chevron-up.svg); width: 10px; height: 10px; }
        QSpinBox#SettingsSpin::down-arrow { image: url(:/icons/chevron-down.svg); width: 10px; height: 10px; }

        /* Custom Checkbox */
        QCheckBox#SettingsCheck {
            background: transparent;
            color: #0F172A;
            font-size: 11px;
            font-weight: 500;
            spacing: 6px;
        }
        QCheckBox#SettingsCheck::indicator {
            width: 14px;
            height: 14px;
            border: 1px solid #CBD5E1;
            border-radius: 2px;
            background-color: #FFFFFF;
        }
        QCheckBox#SettingsCheck::indicator:hover {
            border-color: #94A3B8;
        }
        QCheckBox#SettingsCheck::indicator:checked {
            background-color: #3B82F6;
            border-color: #3B82F6;
            image: url(:/icons/check.svg);
        }

        /* Icon Button (Browse) */
        QPushButton#BrowseButton {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px;
        }
        QPushButton#BrowseButton:hover {
            background-color: #F1F5F9;
            border-color: #94A3B8;
        }
        QPushButton#BrowseButton:pressed {
            background-color: #E2E8F0;
        }

        /* Dialog Action Buttons (Slimmer) */
        QPushButton#ApplyButton {
            background-color: #3B82F6;
            border: 1px solid #2563EB;
            border-radius: 2px;
            color: #FFFFFF;
            font-size: 11px;
            font-weight: 600;
            padding: 2px 14px;
            min-height: 22px; /* COMPACT */
        }
        QPushButton#ApplyButton:hover { background-color: #2563EB; }
        QPushButton#ApplyButton:pressed { background-color: #1D4ED8; }
        QPushButton#ApplyButton:focus { outline: none; }

        QPushButton#CancelButton {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px;
            color: #475569;
            font-size: 11px;
            font-weight: 500;
            padding: 2px 14px;
            min-height: 22px; /* COMPACT */
        }
        QPushButton#CancelButton:hover {
            background-color: #F1F5F9;
            border-color: #94A3B8;
            color: #0F172A;
        }
        QPushButton#CancelButton:pressed { background-color: #E2E8F0; }
    )");
}
