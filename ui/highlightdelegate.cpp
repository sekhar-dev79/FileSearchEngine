#include "highlightdelegate.h"
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QRegularExpression>

HighlightDelegate::HighlightDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void HighlightDelegate::setSearchTerm(const QString &term)
{
    m_searchTerm = term;
}

void HighlightDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Only apply to the "Name" column and when there is an active search term
    if (index.column() != 0 || m_searchTerm.trimmed().isEmpty()) {
        QStyledItemDelegate::paint(painter, opt, index);
        return;
    }

    painter->save();

    // 1. Draw ONLY the background (selection/hover), NOT the default text.
    opt.text = "";
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    // 2. Calculate Icon Space
    // We need to move our text to the right if there's an icon present.
    int iconMargin = 4;
    int iconWidth = 0;
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        iconWidth = opt.decorationSize.width() + iconMargin;
    }

    // 3. Prepare Highlighted Text
    QString originalText = index.data(Qt::DisplayRole).toString();
    QString escapedText = originalText.toHtmlEscaped();
    QString escapedTerm = m_searchTerm.toHtmlEscaped();

    const QString highlightFmt = QStringLiteral("<span style='background-color: rgba(254, 240, 138, 0.8); color: #0F172A; border-radius: 2px;'>\\1</span>");
    QRegularExpression re("(" + QRegularExpression::escape(escapedTerm) + ")", QRegularExpression::CaseInsensitiveOption);
    escapedText.replace(re, highlightFmt);

    // 4. Render with QTextDocument
    QTextDocument doc;
    doc.setHtml(escapedText);
    doc.setDefaultFont(opt.font);

    // 5. Adjust the rectangle to avoid the icon
    QRect textRect = opt.rect;
    // Shift left edge by the icon width + standard padding
    textRect.adjust(8 + iconWidth, 0, -8, 0);

    painter->translate(textRect.topLeft());

    // Center vertically
    int textHeight = static_cast<int>(doc.size().height());
    painter->translate(0, (textRect.height() - textHeight) / 2);

    QAbstractTextDocumentLayout::PaintContext ctx;
    if (opt.state & QStyle::State_Selected) {
        ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::HighlightedText));
    } else {
        ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Text));
    }

    doc.documentLayout()->draw(painter, ctx);
    painter->restore();
}
