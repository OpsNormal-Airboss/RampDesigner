#include "MainWindow.h"
#include "TelemetryManager.h"
#include <QApplication>
#include <QTimer>

static void initResources() { Q_INIT_RESOURCE(aircraft_library); }

int main(int argc, char* argv[]) {
    initResources();
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Airshow Ramp Layout Designer"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("OpsNormal Airboss"));

    MainWindow w;
    w.show();

    QTimer::singleShot(800, &w, [&w]{
        arld::app::TelemetryManager::instance().checkConsent(&w);
    });
    arld::app::TelemetryManager::instance().record(
        arld::app::TelemetryManager::Event::Launch);

    return app.exec();
}
