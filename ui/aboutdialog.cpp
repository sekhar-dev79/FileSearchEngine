// ui/aboutdialog.cpp
#include "aboutdialog.h"
#include "constants.h"

#include <QDate>
#include <QSysInfo>
#include <QIcon>
#include <QFrame>
#include <QGridLayout>

AboutDialog::AboutDialog(int indexedFiles, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("About FileSearchEngine"));
    setModal(true);
    setMinimumWidth(280); // COMPACT: Extra slim dialog

    setupUi(indexedFiles);
    applyStyles();

    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void AboutDialog::setupUi(int indexedFiles)
{
    auto *mainLayout = new QVBoxLayout(this);
    // COMPACT: Minimal margins
    mainLayout->setContentsMargins(16, 16, 16, 12);
    mainLayout->setSpacing(0);

    // ── App icon ──────────────────────────────────────────────
    auto *iconLabel = new QLabel(this);
    // COMPACT: Slightly smaller app icon (48x48)
    iconLabel->setPixmap(QIcon(QStringLiteral(":/icons/app.png")).pixmap(48, 48));
    iconLabel->setAlignment(Qt::AlignHCenter);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(8);

    // ── App name + version ────────────────────────────────────
    auto *nameLabel = new QLabel(QStringLiteral("FileSearchEngine"), this);
    nameLabel->setObjectName(QStringLiteral("AboutTitle"));
    nameLabel->setAlignment(Qt::AlignHCenter);
    mainLayout->addWidget(nameLabel);

    auto *versionLabel = new QLabel(
        QStringLiteral("Version %1").arg(QLatin1StringView(Constants::APP_VERSION)), this);
    versionLabel->setObjectName(QStringLiteral("AboutSubtitle"));
    versionLabel->setAlignment(Qt::AlignHCenter);
    mainLayout->addWidget(versionLabel);
    mainLayout->addSpacing(12);

    // ── The Native QFrame Card ────────────────────────────────
    auto *cardFrame = new QFrame(this);
    cardFrame->setObjectName(QStringLiteral("InfoCard"));

    auto *cardLayout = new QGridLayout(cardFrame);
    // COMPACT: Inner card padding reduced drastically
    cardLayout->setContentsMargins(12, 8, 12, 8);
    cardLayout->setSpacing(6);

    auto *lblFiles = new QLabel(QStringLiteral("Files indexed:"), cardFrame);
    lblFiles->setObjectName(QStringLiteral("CardKey"));
    auto *valFiles = new QLabel(QString::number(indexedFiles), cardFrame);
    valFiles->setObjectName(QStringLiteral("CardValue"));
    cardLayout->addWidget(lblFiles, 0, 0);
    cardLayout->addWidget(valFiles, 0, 1, Qt::AlignRight);

    auto *lblQt = new QLabel(QStringLiteral("Qt version:"), cardFrame);
    lblQt->setObjectName(QStringLiteral("CardKey"));
    auto *valQt = new QLabel(QLatin1StringView(qVersion()), cardFrame);
    valQt->setObjectName(QStringLiteral("CardValue"));
    cardLayout->addWidget(lblQt, 1, 0);
    cardLayout->addWidget(valQt, 1, 1, Qt::AlignRight);

    auto *lblDate = new QLabel(QStringLiteral("Build date:"), cardFrame);
    lblDate->setObjectName(QStringLiteral("CardKey"));
    auto *valDate = new QLabel(QLatin1StringView(__DATE__), cardFrame);
    valDate->setObjectName(QStringLiteral("CardValue"));
    cardLayout->addWidget(lblDate, 2, 0);
    cardLayout->addWidget(valDate, 2, 1, Qt::AlignRight);

    auto *lblPlat = new QLabel(QStringLiteral("Platform:"), cardFrame);
    lblPlat->setObjectName(QStringLiteral("CardKey"));
    auto *valPlat = new QLabel(QSysInfo::prettyProductName(), cardFrame);
    valPlat->setObjectName(QStringLiteral("CardValue"));
    valPlat->setMinimumWidth(120);
    cardLayout->addWidget(lblPlat, 3, 0);
    cardLayout->addWidget(valPlat, 3, 1, Qt::AlignRight);

    mainLayout->addWidget(cardFrame);
    mainLayout->addSpacing(16);

    // ── Close button ──────────────────────────────────────────
    auto *btnLayout = new QHBoxLayout();
    auto *closeBtn = new QPushButton(QStringLiteral("Close"), this);
    closeBtn->setObjectName(QStringLiteral("AboutCloseBtn"));
    closeBtn->setDefault(true);
    closeBtn->setFixedWidth(80); // COMPACT: Slimmer button

    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void AboutDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #F8FAFC;
        }
        QLabel {
            background: transparent;
            font-family: "Segoe UI", "Inter", sans-serif;
        }
        QLabel#AboutTitle {
            font-size: 15px; /* COMPACT: Smaller title */
            font-weight: 700;
            color: #0F172A;
        }
        QLabel#AboutSubtitle {
            font-size: 11px;
            color: #64748B;
            font-weight: 500;
        }

        /* The Native Frame Card */
        QFrame#InfoCard {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px; /* COMPACT: Sharp corners */
        }
        QLabel#CardKey {
            color: #64748B;
            font-size: 11px; /* COMPACT: 11px global font */
        }
        QLabel#CardValue {
            color: #0F172A;
            font-size: 11px;
            font-weight: 600;
        }

        /* Minimalist Button */
        QPushButton#AboutCloseBtn {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px; /* COMPACT */
            color: #0F172A;
            padding: 4px 12px;
            font-size: 11px;
            font-weight: 500;
        }
        QPushButton#AboutCloseBtn:hover {
            background-color: #F1F5F9;
            border-color: #94A3B8;
        }
        QPushButton#AboutCloseBtn:pressed {
            background-color: #E2E8F0;
        }
        QPushButton#AboutCloseBtn:focus {
            outline: none;
            border: 1px solid #3B82F6;
        }
    )");
}
