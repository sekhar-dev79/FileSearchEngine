#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>

class SearchBar : public QWidget
{
    Q_OBJECT
public:
    explicit SearchBar(QWidget *parent = nullptr);

    QLineEdit* lineEdit() const { return m_lineEdit; }
    bool isRegexMode() const { return m_regexButton && m_regexButton->isChecked(); }

    QString text() const;
    void clear();
    void selectAll();
    void setFocus();


signals:
    void searchTextChanged(const QString &text);
    void regexToggled(bool enabled);

private:
    QLineEdit *m_lineEdit;
    QHBoxLayout *m_layout;
    QPushButton *m_regexButton;
};
