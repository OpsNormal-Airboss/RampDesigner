#include "MainWindow.h"
#include <QApplication>

// Force the aircraft library QRC static initializer to be linked in.
// Without this, the linker strips qrc_aircraft_library.cpp from libarld_ui.a
// (no other symbol references it) and :/library/* resources don't exist at runtime.
static void initResources() { Q_INIT_RESOURCE(aircraft_library); }

int main(int argc, char* argv[]) {
    initResources();
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Airshow Ramp Layout Designer"));
    app.setApplicationVersion(QStringLiteral("0.1.0-poc"));
    app.setOrganizationName(QStringLiteral("ARLD"));

    MainWindow w;
    w.show();
    return app.exec();
}
