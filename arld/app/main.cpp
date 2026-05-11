#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Airshow Ramp Layout Designer"));
    app.setApplicationVersion(QStringLiteral("0.1.0-poc"));
    app.setOrganizationName(QStringLiteral("ARLD"));
    // Sprint 0-2: main window created here
    return 0;
}
