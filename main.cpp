#include <QApplication>
#include <QTimer>
#include <QIcon>
#include "ui/mainwindow.h"
#include "utils/constants.h"
#include "models/fileitem.h"

int main(int argc, char *argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName(QLatin1StringView(Constants::APP_NAME));
    app.setApplicationVersion(QLatin1StringView(Constants::APP_VERSION));
    app.setOrganizationName(QStringLiteral("ChanduVerse"));

    // ── App icon ──────────────────────────────────────────────
    // Set the application-wide icon — appears in the title bar,
    // Alt+Tab switcher, and Windows taskbar button.
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    // ── Keep app alive when window is hidden ──────────────────
    // Without this, hiding the main window would quit the app.
    // Setting this to false lets the tray icon keep the app running [web:331].
    app.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<FileItem>("FileItem");
    qRegisterMetaType<QVector<FileItem>>("QVector<FileItem>");

    MainWindow window;
    window.show();

    QTimer::singleShot(0, &window, [&window]() {
        window.startIndexing();
    });

    return app.exec();
}
