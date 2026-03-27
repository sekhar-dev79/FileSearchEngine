// ui/previewpanel.h
#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include "fileitem.h"

class PreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(QWidget *parent = nullptr);

public slots:
    void setFile(const FileItem &item);
    void clearPreview();

private:
    void setupUi();
    void loadTextPreview(const QString &path);
    void loadImagePreview(const QString &path);
    void loadIconPreview(const QString &path);

    QVBoxLayout *m_mainLayout;

    QLabel      *m_iconLabel;
    QLabel      *m_nameLabel;
    QLabel      *m_metaLabel;

    QTextEdit   *m_textPreview;
    QLabel      *m_imagePreview;
    QLabel      *m_placeholderLabel;

    // ── Quick Actions ──
    QHBoxLayout *m_actionLayout;
    QPushButton *m_btnOpen;
    QPushButton *m_btnExplorer;
    QPushButton *m_btnCopy;

    QString m_currentPath;
    bool m_isImagePreview;
};
