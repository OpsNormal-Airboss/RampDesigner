#pragma once
#include <QString>
#include <QWidget>

namespace arld::app {

// Not a QObject — static QObject singletons crash on macOS during exit because
// ~QObject() runs after QApplication tears down the platform layer (issue #8).
class TelemetryManager {
public:
    enum class Event { Launch, ProjectSave, ProjectOpen,
                       ExportSvg, ExportPdf, ExportPng, ExportJpeg, ExportBatch };

    static TelemetryManager& instance();

    /// Show first-run consent dialog if user has not yet decided.
    /// Safe to call with parent=nullptr.
    void checkConsent(QWidget* parent = nullptr);

    bool isConsented() const;

    /// Increment local event counter if consent has been given.
    void record(Event e);

    /// Human-readable usage stats for display in Help menu.
    QString stats() const;

private:
    TelemetryManager() = default;

    static const char* eventKey(Event e);
};

} // namespace arld::app
