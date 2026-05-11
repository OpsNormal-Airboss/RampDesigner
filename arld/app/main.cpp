#include "MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Airshow Ramp Layout Designer"));
    app.setApplicationVersion(QStringLiteral("0.1.0-poc"));
    app.setOrganizationName(QStringLiteral("ARLD"));

    MainWindow w;
    w.show();
    return app.exec();
}
