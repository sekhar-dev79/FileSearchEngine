// ui/resultview.cpp
#include "resultview.h"
#include "filemodel.h"

#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>


ResultView::ResultView(QWidget *parent) : QTableView(parent)
{
    setObjectName(QStringLiteral("ResultView"));

    // ── Core Table Behavior ───────────────────────────────────
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);
    setShowGrid(false);
    setMouseTracking(true); // Enables hover effects

    // CRITICAL UX: Prevent rows from expanding vertically if paths are long
    setWordWrap(false);

    // CRITICAL UX: Truncates paths in the middle (C:\...\file.txt)
    setTextElideMode(Qt::ElideMiddle);

    // ── Scrolling & Performance ───────────────────────────────
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // ── Vertical Header (Row Numbers) ─────────────────────────
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(26); // Compact, dense row height

    // ── Horizontal Header (Column Titles) ─────────────────────
    QHeaderView *hHeader = horizontalHeader();
    hHeader->setStretchLastSection(true);
    hHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // UI POLISH: Prevents the header from turning bold/blue when a row is clicked
    hHeader->setHighlightSections(false);

    // UI POLISH: Removes the dotted focus box if the user clicks the header
    hHeader->setFocusPolicy(Qt::NoFocus);

    hHeader->setSectionsClickable(true);
    setSortingEnabled(true); // Must be called after setting up headers

    // ── Context Menu Setup ────────────────────────────────────
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QTableView::customContextMenuRequested,
            this, [this](const QPoint &pos) {
                const QModelIndex index = indexAt(pos);
                if (!index.isValid()) return;

                // Ensure the row is fully selected before showing the menu
                selectionModel()->setCurrentIndex(
                    index,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

                emit contextMenuRequestedForRow(
                    index.row(), viewport()->mapToGlobal(pos));
            });
}

// ── keyPressEvent ─────────────────────────────────────────────────────────────
void ResultView::keyPressEvent(QKeyEvent *event)
{
    const int row = currentIndex().row();
    const bool hasSelection = (row >= 0);

    switch (event->key())
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // Open the selected file — same as double-click.
        if (hasSelection)
        {
            emit enterPressedOnRow(row);
            event->accept();  // mark as handled — stop further propagation
            return;
        }
        break;

    case Qt::Key_C:
        // Ctrl+C → copy full path to clipboard.
        if (event->modifiers() & Qt::ControlModifier && hasSelection)
        {
            emit copyPressedOnRow(row);
            event->accept();
            return;
        }
        break;

    default:
        break;
    }

    // Forward everything else (arrows, Page Up/Down, Home, End)
    // to QTableView so normal keyboard navigation still works.
    QTableView::keyPressEvent(event);
}

// ── Drag and Drop Implementation ──────────────────────────────────────────────

void ResultView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
    }
    // Always call base class so normal selection still works!
    QTableView::mousePressEvent(event);
}

void ResultView::mouseMoveEvent(QMouseEvent *event)
{
    // 1. Only start drag if the Left Mouse Button is held down
    if (!(event->buttons() & Qt::LeftButton)) {
        return QTableView::mouseMoveEvent(event);
    }

    // 2. Prevent accidental drags by checking the drag distance threshold
    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
        return QTableView::mouseMoveEvent(event);
    }

    // 3. Get the currently selected row
    const int row = currentIndex().row();
    if (row < 0) return;

    // 4. Retrieve the file path from our custom model
    auto *fileModel = qobject_cast<FileModel*>(model());
    if (!fileModel) return;

    const FileItem &item = fileModel->fileAt(row);

    // 5. Create the MimeData (The universal format the OS uses for Drag & Drop)
    auto *mimeData = new QMimeData;
    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(item.path));
    mimeData->setUrls(urls); // This tells Windows/macOS/Linux "Here is a file"

    // 6. Execute the Drag
    auto *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Optional: Set a nice icon during the drag
    // drag->setPixmap(QIcon(":/icons/file-text.svg").pixmap(32, 32));

    drag->exec(Qt::CopyAction | Qt::LinkAction);
}

void ResultView::paintEvent(QPaintEvent *event)
{
    // Draw the normal table first
    QTableView::paintEvent(event);

    // If the model exists but has 0 rows, draw our beautiful watermark!
    if (model() && model()->rowCount() == 0)
    {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        // Premium soft slate color
        painter.setPen(QColor("#94A3B8"));

        QFont watermarkFont = font();
        watermarkFont.setPointSize(14);
        watermarkFont.setWeight(QFont::Medium);
        painter.setFont(watermarkFont);

        // Center the text in the empty white space
        painter.drawText(viewport()->rect(), Qt::AlignCenter,
                         QStringLiteral("No files found matching your criteria."));
    }
}
