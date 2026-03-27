// ui/filterbar.cpp
#include "filterbar.h"
#include <QDateTime>
#include <QIcon>
#include <QSignalBlocker>

struct CategoryDef {
    QString name;
    QString iconPath;
    QStringList extensions;
};

static const QVector<CategoryDef> CATEGORIES = {
    { "Images", ":/icons/image.svg", { "png", "jpg", "jpeg", "gif", "bmp", "webp", "svg", "ico" } },
    { "Documents", ":/icons/file-text.svg", { "pdf", "doc", "docx", "txt", "rtf", "md", "csv", "xls", "xlsx", "ppt", "pptx" } },
    { "Source Code", ":/icons/code.svg", { "cpp", "c", "h", "hpp", "js", "ts", "py", "java", "cs", "html", "css", "json", "xml", "qml", "pro" } },
    { "Audio & Video", ":/icons/circle-play.svg", { "mp3", "wav", "flac", "mp4", "mkv", "avi", "mov", "webm" } },
    { "Archives & Executables", ":/icons/package.svg", { "zip", "rar", "7z", "tar", "gz", "exe", "msi", "dll", "bat", "sh" } }
};

FilterBar::FilterBar(QWidget *parent)
    : QWidget(parent)
{
    m_mainLayout = new QHBoxLayout(this);
    // COMPACT: Tighter margins
    m_mainLayout->setContentsMargins(0, 2, 0, 2);
    // COMPACT: Reduced gap between the filter groups
    m_mainLayout->setSpacing(16);

    // ── Group 1: Cascading Extension Filter ───────────────────
    auto *typeLayout = new QHBoxLayout();
    typeLayout->setSpacing(4); // COMPACT: Tighter label-to-box gap

    m_typeLabel = new QLabel(QStringLiteral("Type:"), this);
    m_typeLabel->setObjectName(QStringLiteral("FilterLabel"));

    // 1A. Category Dropdown
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setMinimumWidth(130); // COMPACT: Scaled down
    m_categoryCombo->setToolTip(QStringLiteral("Select file category"));
    m_categoryCombo->addItem(QIcon(":/icons/layout-grid.svg"), QStringLiteral("All Categories"), QStringList());
    for (const auto &cat : CATEGORIES) {
        m_categoryCombo->addItem(QIcon(cat.iconPath), cat.name, cat.extensions);
    }

    // 1B. Specific Extension Dropdown
    m_extCombo = new QComboBox(this);
    m_extCombo->setMinimumWidth(100); // COMPACT: Scaled down
    m_extCombo->setToolTip(QStringLiteral("Select specific extension"));
    m_extCombo->addItem(QIcon(":/icons/file.svg"), QStringLiteral("All extensions"), QStringList());

    typeLayout->addWidget(m_typeLabel);
    typeLayout->addWidget(m_categoryCombo);
    typeLayout->addWidget(m_extCombo);

    // ── Group 2: Size filter ──────────────────────────────────
    auto *sizeLayout = new QHBoxLayout();
    sizeLayout->setSpacing(4);
    m_sizeLabel = new QLabel(QStringLiteral("Min size:"), this);
    m_sizeLabel->setObjectName(QStringLiteral("FilterLabel"));

    m_sizeSpinBox = new QSpinBox(this);
    m_sizeSpinBox->setRange(0, 1'000'000);
    m_sizeSpinBox->setValue(0);
    m_sizeSpinBox->setSuffix(QStringLiteral(" KB"));
    m_sizeSpinBox->setSpecialValueText(QStringLiteral("Any size"));
    m_sizeSpinBox->setMinimumWidth(90); // COMPACT: Scaled down
    m_sizeSpinBox->setToolTip(QStringLiteral("Show files at least this large"));

    sizeLayout->addWidget(m_sizeLabel);
    sizeLayout->addWidget(m_sizeSpinBox);

    // ── Group 3: Date filter ──────────────────────────────────
    auto *dateLayout = new QHBoxLayout();
    dateLayout->setSpacing(4);
    m_dateLabel = new QLabel(QStringLiteral("Modified:"), this);
    m_dateLabel->setObjectName(QStringLiteral("FilterLabel"));

    m_dateCombo = new QComboBox(this);
    m_dateCombo->setMinimumWidth(100); // COMPACT: Scaled down
    m_dateCombo->setToolTip(QStringLiteral("Filter by last modified date"));
    m_dateCombo->addItem(QStringLiteral("Any time"),    0);
    m_dateCombo->addItem(QStringLiteral("Today"),       1);
    m_dateCombo->addItem(QStringLiteral("This week"),   7);
    m_dateCombo->addItem(QStringLiteral("This month"),  30);
    m_dateCombo->addItem(QStringLiteral("This year"),   365);

    dateLayout->addWidget(m_dateLabel);
    dateLayout->addWidget(m_dateCombo);

    // ── Reset button ──────────────────────────────────────────
    m_resetButton = new QPushButton(QStringLiteral("Reset Filters"), this);
    m_resetButton->setObjectName(QStringLiteral("ResetButton"));
    m_resetButton->setToolTip(QStringLiteral("Clear all active filters"));
    m_resetButton->setEnabled(false);

    // ── Layout assembly ───────────────────────────────────────
    m_mainLayout->addLayout(typeLayout);
    m_mainLayout->addLayout(sizeLayout);
    m_mainLayout->addLayout(dateLayout);
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_resetButton);

    // ── Connections ───────────────────────────────────────────
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterBar::onCategoryChanged);
    connect(m_extCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterBar::onAnyFilterChanged);
    connect(m_sizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &FilterBar::onAnyFilterChanged);
    connect(m_dateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterBar::onAnyFilterChanged);
    connect(m_resetButton, &QPushButton::clicked, this, &FilterBar::resetFilters);

    applyStyles();
}

void FilterBar::onCategoryChanged()
{
    QSignalBlocker blocker(m_extCombo);
    m_extCombo->clear();
    m_extCombo->addItem(QIcon(":/icons/file.svg"), QStringLiteral("All extensions"), QStringList());

    const QStringList catExts = m_categoryCombo->currentData().toStringList();

    for (const QString &ext : m_allScannedExtensions) {
        if (ext.isEmpty()) continue;
        if (!catExts.isEmpty() && !catExts.contains(ext, Qt::CaseInsensitive)) continue;

        m_extCombo->addItem(QIcon(":/icons/file.svg"), QStringLiteral(".%1 files").arg(ext.toUpper()), QStringList{ext});
    }

    onAnyFilterChanged();
}

void FilterBar::onAnyFilterChanged()
{
    m_resetButton->setEnabled(!isDefault());
    emit filtersChanged();
}

FilterCriteria FilterBar::criteria() const
{
    FilterCriteria c;

    if (m_extCombo->currentIndex() > 0) {
        c.extensions = m_extCombo->currentData().toStringList();
    } else {
        c.extensions = m_categoryCombo->currentData().toStringList();
    }

    const int kb = m_sizeSpinBox->value();
    c.minSizeBytes = (kb > 0) ? static_cast<qint64>(kb) * 1024LL : 0LL;

    const int days = m_dateCombo->currentData().toInt();
    if (days > 0) {
        c.modifiedAfter = QDateTime::currentDateTime().addDays(-days);
    }

    return c;
}

void FilterBar::updateExtensions(const QVector<QString> &extensions)
{
    m_allScannedExtensions = extensions;
    onCategoryChanged();
}

void FilterBar::resetFilters()
{
    QSignalBlocker b1(m_categoryCombo);
    QSignalBlocker b2(m_extCombo);
    QSignalBlocker b3(m_sizeSpinBox);
    QSignalBlocker b4(m_dateCombo);

    m_categoryCombo->setCurrentIndex(0);
    m_sizeSpinBox->setValue(0);
    m_dateCombo->setCurrentIndex(0);

    onCategoryChanged();
}

bool FilterBar::isDefault() const
{
    return m_categoryCombo->currentIndex() == 0
           && m_extCombo->currentIndex()   == 0
           && m_sizeSpinBox->value()       == 0
           && m_dateCombo->currentIndex()  == 0;
}

void FilterBar::applyStyles()
{
    setStyleSheet(R"(
        /* ── Labels ── */
        QLabel#FilterLabel {
            color: #475569;
            font-size: 11px; /* COMPACT */
            font-weight: 600;
        }

        /* ── ComboBox & SpinBox Base (Slimmed Down) ── */
        QComboBox, QSpinBox {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px; /* COMPACT: Sharp corners */
            padding: 2px 20px 2px 6px; /* Reduced internal padding */
            min-height: 20px; /* COMPACT: Shorter height */
            color: #0F172A;
            font-size: 11px; /* COMPACT: 11px global font */
        }
        QComboBox:hover, QSpinBox:hover { border-color: #94A3B8; }
        QComboBox:focus, QSpinBox:focus {
            border: 1px solid #3B82F6;
        }

        /* ── QComboBox Dropdown ── */
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 18px; /* COMPACT */
            border-left: 1px solid #E2E8F0;
            background-color: #F8FAFC;
        }
        QComboBox::drop-down:hover { background-color: #E2E8F0; }
        QComboBox::down-arrow {
            image: url(:/icons/chevron-down.svg);
            width: 10px; height: 10px; /* COMPACT: smaller icon */
        }
        QComboBox::down-arrow:on { top: 1px; }

        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 2px;
            selection-background-color: #E0E7FF;
            selection-color: #1E40AF;
            padding: 2px;
            outline: none;
            font-size: 11px;
        }

        QComboBox QAbstractItemView::item {
            min-height: 20px; /* COMPACT: Tighter list rows */
            padding: 2px 4px;
        }

        /* ── QSpinBox Up/Down Buttons ── */
        QSpinBox { padding: 2px 18px 2px 6px; }
        QSpinBox::up-button, QSpinBox::down-button {
            subcontrol-origin: border;
            width: 16px; /* COMPACT */
            background: #F8FAFC;
            border-left: 1px solid #E2E8F0;
        }
        QSpinBox::up-button {
            subcontrol-position: top right;
            border-bottom: 1px solid #E2E8F0;
        }
        QSpinBox::down-button {
            subcontrol-position: bottom right;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #E2E8F0; }
        QSpinBox::up-arrow { image: url(:/icons/chevron-up.svg); width: 8px; height: 8px; }
        QSpinBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 8px; height: 8px; }
        QSpinBox::up-arrow:disabled, QSpinBox::up-arrow:off,
        QSpinBox::down-arrow:disabled, QSpinBox::down-arrow:off { opacity: 0.3; }

        /* ── Dynamic Ghost Reset Button ── */
        QPushButton#ResetButton {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 2px;
            color: #64748B;
            padding: 2px 10px;
            font-size: 11px;
            font-weight: 600;
        }
        QPushButton#ResetButton:hover {
            background-color: #FEF2F2;
            border: 1px solid #FECACA;
            color: #DC2626;
        }
        QPushButton#ResetButton:pressed { background-color: #FEE2E2; }
        QPushButton#ResetButton:disabled { color: transparent; border: none; }
    )");
}
