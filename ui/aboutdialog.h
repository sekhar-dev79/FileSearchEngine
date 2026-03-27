// ui/aboutdialog.h
//
// A simple modal dialog showing application metadata.
// Opened via Help → About or Ctrl+Shift+A.
// Uses QLabel with rich text (HTML subset) for formatted display —
// QLabel supports a limited HTML subset without needing QtWebEngine.

#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    // indexedFiles — passed from MainWindow so the dialog shows live stats.
    explicit AboutDialog(int indexedFiles, QWidget *parent = nullptr);

private:
    void setupUi(int indexedFiles);
    void applyStyles();
};
