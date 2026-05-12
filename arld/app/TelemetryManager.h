#pragma once
#include <QObject>
#include <QString>
#include <QWidget>

namespace arld::app {

class TelemetryManager : public QObject {
    Q_OBJECT
public:
    enum class Event { Launch, ProjectSave, ProjectOpen,
                       ExportSvg, ExportPdf, ExportPng, ExportJpeg, ExportBatch };
    Q_ENUM(Event)

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
};

} // namespace arld::app
