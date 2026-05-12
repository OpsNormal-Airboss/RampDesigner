#include "TelemetryManager.h"
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStringList>

namespace arld::app {

TelemetryManager& TelemetryManager::instance() {
    static TelemetryManager s;
    return s;
}

const char* TelemetryManager::eventKey(Event e) {
    switch (e) {
        case Event::Launch:       return "Launch";
        case Event::ProjectSave:  return "ProjectSave";
        case Event::ProjectOpen:  return "ProjectOpen";
        case Event::ExportSvg:    return "ExportSvg";
        case Event::ExportPdf:    return "ExportPdf";
        case Event::ExportPng:    return "ExportPng";
        case Event::ExportJpeg:   return "ExportJpeg";
        case Event::ExportBatch:  return "ExportBatch";
    }
    return "Unknown";
}

void TelemetryManager::checkConsent(QWidget* parent) {
    QSettings s(QStringLiteral("OpsNormal"), QStringLiteral("ARLD"));
    if (s.value(QStringLiteral("telemetry/askedAlready"), false).toBool())
        return;

    QMessageBox dlg(parent);
    dlg.setWindowTitle(QStringLiteral("Help Improve ARLD"));
    dlg.setText(QStringLiteral(
        "Would you like to send anonymous usage statistics to help "
        "improve ARLD?\n\nNo personal data, file contents, or network "
        "connections are involved — usage counts are stored locally "
        "only. You can change this setting in Help → Preferences."));
    dlg.setIcon(QMessageBox::Question);

    QPushButton* yesBtn   = dlg.addButton(QStringLiteral("Yes, help improve ARLD"),
                                           QMessageBox::AcceptRole);
    QPushButton* noBtn    = dlg.addButton(QStringLiteral("No thanks"),
                                           QMessageBox::RejectRole);
    /*QPushButton* laterBtn =*/ dlg.addButton(QStringLiteral("Ask me later"),
                                           QMessageBox::NoRole);
    dlg.setDefaultButton(yesBtn);

    dlg.exec();

    const QAbstractButton* clicked = dlg.clickedButton();
    if (clicked == yesBtn) {
        s.setValue(QStringLiteral("telemetry/consented"),    true);
        s.setValue(QStringLiteral("telemetry/askedAlready"), true);
    } else if (clicked == noBtn) {
        s.setValue(QStringLiteral("telemetry/consented"),    false);
        s.setValue(QStringLiteral("telemetry/askedAlready"), true);
    }
    // "Ask me later" leaves askedAlready=false so dialog appears next launch
}

bool TelemetryManager::isConsented() const {
    QSettings s(QStringLiteral("OpsNormal"), QStringLiteral("ARLD"));
    return s.value(QStringLiteral("telemetry/consented"), false).toBool();
}

void TelemetryManager::record(Event e) {
    if (!isConsented()) return;
    QSettings s(QStringLiteral("OpsNormal"), QStringLiteral("ARLD"));
    const QString key = QStringLiteral("telemetry/events/") + QLatin1StringView(eventKey(e));
    s.setValue(key, s.value(key, 0).toInt() + 1);
}

QString TelemetryManager::stats() const {
    if (!isConsented())
        return QStringLiteral("Usage statistics: not collecting (opt-in disabled).");
    QSettings s(QStringLiteral("OpsNormal"), QStringLiteral("ARLD"));
    static constexpr Event allEvents[] = {
        Event::Launch, Event::ProjectSave, Event::ProjectOpen,
        Event::ExportSvg, Event::ExportPdf, Event::ExportPng,
        Event::ExportJpeg, Event::ExportBatch
    };
    QStringList lines;
    for (Event e : allEvents) {
        const QString key = QStringLiteral("telemetry/events/") + QLatin1StringView(eventKey(e));
        const int count = s.value(key, 0).toInt();
        if (count > 0)
            lines << QStringLiteral("%1: %2").arg(QLatin1StringView(eventKey(e))).arg(count);
    }
    return lines.isEmpty()
        ? QStringLiteral("No events recorded yet.")
        : lines.join(QStringLiteral("\n"));
}

} // namespace arld::app
