// ui/resultview.h
#pragma once

#include <QTableView>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPoint>

class ResultView : public QTableView
{
    Q_OBJECT

public:
    explicit ResultView(QWidget *parent = nullptr);

signals:
    void contextMenuRequestedForRow(int row, const QPoint &globalPos);
    void enterPressedOnRow(int row);
    void copyPressedOnRow(int row);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint m_dragStartPos;
};
