#include "searchbar.h"

SearchBar::SearchBar(QWidget *parent) : QWidget(parent)
{
    m_layout   = new QHBoxLayout(this);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setPlaceholderText(
        QStringLiteral("Search files by name... (e.g., *.cpp, report)"));
    m_lineEdit->setClearButtonEnabled(true);
    m_lineEdit->setObjectName(QStringLiteral("SearchLineEdit"));
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_lineEdit);

    m_regexButton = new QPushButton(QStringLiteral(".*"), this);
    m_regexButton->setCheckable(true);
    m_regexButton->setFixedWidth(32);
    m_regexButton->setToolTip(QStringLiteral("Enable Regular Expression Search"));
    m_regexButton->setObjectName("RegexToggle");
    m_layout->addWidget(m_regexButton);

    setLayout(m_layout);

    connect(m_lineEdit, &QLineEdit::textChanged,
            this,       &SearchBar::searchTextChanged);
}

QString SearchBar::text() const { return m_lineEdit->text(); }
void    SearchBar::setFocus()   { m_lineEdit->setFocus(); }
void SearchBar::clear()     { m_lineEdit->clear(); }
void SearchBar::selectAll() { m_lineEdit->selectAll(); }

