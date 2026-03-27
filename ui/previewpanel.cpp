// ui/previewpanel.cpp
#include "previewpanel.h"
#include "helpers.h"
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>
#include <QDir>
#include <QSet> // Added for fast extension lookups

PreviewPanel::PreviewPanel(QWidget *parent) : QWidget(parent)
{
    setupUi();
    clearPreview();
}

void PreviewPanel::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    // COMPACT: Drastically reduced margins
    m_mainLayout->setContentsMargins(8, 12, 8, 8);
    m_mainLayout->setSpacing(8);

    // ── File Header (Icon + Name) ──
    m_iconLabel = new QLabel(this);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setWordWrap(true);
    // COMPACT: Smaller, crisper title font
    m_nameLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600; color: #0F172A;"));
    m_nameLabel->setAlignment(Qt::AlignCenter);

    m_metaLabel = new QLabel(this);
    m_metaLabel->setAlignment(Qt::AlignCenter);
    // COMPACT: 11px meta text
    m_metaLabel->setStyleSheet(QStringLiteral("color: #64748B; font-size: 11px;"));

    // ── Preview Areas ──
    m_textPreview = new QTextEdit(this);
    m_textPreview->setReadOnly(true);
    // COMPACT: Sharp 2px corners, 11px code font, tighter padding
    m_textPreview->setStyleSheet(QStringLiteral(
        "background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 2px; "
        "padding: 4px; color: #334155; font-family: 'Courier New', monospace; font-size: 11px;"
        ));

    m_imagePreview = new QLabel(this);
    m_imagePreview->setAlignment(Qt::AlignCenter);
    m_imagePreview->setStyleSheet(QStringLiteral(
        "background-color: #FFFFFF; border: 1px solid #CBD5E1; border-radius: 2px;"
        ));

    m_placeholderLabel = new QLabel(QStringLiteral("Select a file to preview"), this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setStyleSheet(QStringLiteral("color: #94A3B8; font-style: italic; font-size: 11px;"));

    m_mainLayout->addWidget(m_iconLabel);
    m_mainLayout->addWidget(m_nameLabel);
    m_mainLayout->addWidget(m_metaLabel);
    m_mainLayout->addWidget(m_textPreview, 1);
    m_mainLayout->addWidget(m_imagePreview, 1);
    m_mainLayout->addWidget(m_placeholderLabel, 1);

    // ── Quick Action Buttons (Bottom Row) ──
    m_actionLayout = new QHBoxLayout();
    m_actionLayout->setSpacing(4); // COMPACT: tighter button grouping

    m_btnOpen = new QPushButton(QIcon(":/icons/external-link.svg"), QStringLiteral("Open"), this);
    m_btnExplorer = new QPushButton(QIcon(":/icons/folder-open.svg"), QStringLiteral("Show in Folder"), this);
    m_btnCopy = new QPushButton(QIcon(":/icons/copy.svg"), QStringLiteral("Copy Path"), this);

    // COMPACT: Sharp borders, 11px font, less padding, subtle outlines
    const QString btnStyle = R"(
        QPushButton {
            background-color: #F8FAFC;
            border: 1px solid #CBD5E1;
            border-radius: 2px;
            padding: 4px 8px;
            color: #475569;
            font-weight: 600;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: #F1F5F9;
            color: #0F172A;
            border-color: #94A3B8;
        }
        QPushButton:pressed {
            background-color: #E2E8F0;
        }
    )";

    for (auto *btn : {m_btnOpen, m_btnExplorer, m_btnCopy}) {
        btn->setStyleSheet(btnStyle);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setIconSize(QSize(14, 14)); // COMPACT: slightly smaller icons
        btn->hide();
        m_actionLayout->addWidget(btn);
    }

    m_mainLayout->addLayout(m_actionLayout);

    // ── Action Logic ──
    connect(m_btnOpen, &QPushButton::clicked, this, [this]() {
        if (!m_currentPath.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(m_currentPath));
    });

    connect(m_btnExplorer, &QPushButton::clicked, this, [this]() {
        if (m_currentPath.isEmpty()) return;
#if defined(Q_OS_WIN)
        QProcess::startDetached("explorer", {"/select,", QDir::toNativeSeparators(m_currentPath)});
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_currentPath).absolutePath()));
#endif
    });

    connect(m_btnCopy, &QPushButton::clicked, this, [this]() {
        if (m_currentPath.isEmpty()) return;
        if (m_isImagePreview) {
            QGuiApplication::clipboard()->setPixmap(QPixmap(m_currentPath));
        } else {
            QGuiApplication::clipboard()->setText(m_currentPath);
        }
    });
}

void PreviewPanel::clearPreview()
{
    m_iconLabel->hide();
    m_nameLabel->hide();
    m_metaLabel->hide();
    m_textPreview->hide();
    m_imagePreview->hide();

    m_btnOpen->hide();
    m_btnExplorer->hide();
    m_btnCopy->hide();

    m_placeholderLabel->show();
    m_currentPath.clear();
}

void PreviewPanel::setFile(const FileItem &item)
{
    m_placeholderLabel->hide();
    m_currentPath = item.path;
    m_isImagePreview = false;

    // 1. Load Header
    static QFileIconProvider provider;
    QIcon icon = provider.icon(QFileInfo(item.path));

    m_iconLabel->setPixmap(icon.pixmap(48, 48));
    m_iconLabel->show();

    m_nameLabel->setText(item.name);
    m_nameLabel->show();

    m_metaLabel->setText(QStringLiteral("%1  |  Modified: %2")
                             .arg(Helpers::formatFileSize(item.size))
                             .arg(item.lastModified.toString("MMM d, yyyy")));
    m_metaLabel->show();

    // Show buttons
    m_btnOpen->show();
    m_btnExplorer->show();
    m_btnCopy->show();

    // ── Extensive File Format Dictionaries ──
    static const QSet<QString> imageExts = {
        "png", "jpg", "jpeg", "webp", "bmp", "gif", "svg", "ico", "tif", "tiff"
    };

    static const QSet<QString> textExts = {
        // Standard Text
        "txt", "md", "csv", "log", "rtf",
        // Web & Data
        "html", "htm", "css", "xml", "json", "yml", "yaml", "toml", "ini", "cfg", "conf",
        // C / C++ / Qt
        "c", "cpp", "cxx", "h", "hpp", "pro", "pri", "qml", "cmake",
        // Other Programming Languages
        "js", "ts", "py", "java", "cs", "rs", "go", "php", "rb", "swift", "kt", "dart", "sql", "sh", "bat", "ps1"
    };

    QString ext = item.extension().toLower();

    // Determine Preview Type
    if (imageExts.contains(ext)) {
        m_isImagePreview = true;
        m_btnCopy->setText(QStringLiteral("Copy Image"));
        m_btnCopy->setIcon(QIcon(":/icons/image.svg"));
        loadImagePreview(item.path);
    }
    else {
        m_btnCopy->setText(QStringLiteral("Copy Path"));
        m_btnCopy->setIcon(QIcon(":/icons/copy.svg"));

        if (textExts.contains(ext)) {
            loadTextPreview(item.path);
        } else {
            // Unrecognized/Binary file (like PDF, EXE, ZIP)
            m_textPreview->hide();
            m_imagePreview->hide();
        }
    }
}

void PreviewPanel::loadTextPreview(const QString &path)
{
    m_imagePreview->hide();
    m_textPreview->clear();

    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString previewText;
        int lines = 0;

        // Read up to 40 lines
        while (!in.atEnd() && lines < 40) {
            previewText += in.readLine() + "\n";
            lines++;
        }

        if (!in.atEnd()) {
            previewText += "\n... [File truncated for preview] ...";
        }

        m_textPreview->setPlainText(previewText);
        m_textPreview->show();
    } else {
        m_textPreview->hide();
    }
}

void PreviewPanel::loadImagePreview(const QString &path)
{
    m_textPreview->hide();

    QPixmap pixmap(path);
    if (!pixmap.isNull()) {
        m_imagePreview->setPixmap(pixmap.scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imagePreview->show();
    } else {
        m_imagePreview->hide();
    }
}
