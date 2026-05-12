#include "TelemetryManager.h"
#include <QMessageBox>
#include <QMetaEnum>
#include <QPushButton>
#include <QSettings>
#include <QStringList>

namespace arld::app {

TelemetryManager& TelemetryManager::instance() {
    static TelemetryManager s;
    return s;
}

void TelemetryManager::checkConsent(QWidget* parent) {
    QSettings s(QStringLiteral("OpsNormal"), QStringLiteral("ARLD"));
    if (s.value(QStringLiteral("telemetry/askedAlready"), false).toBool())
        return;

    QMessageBox dlg(parent);
    dlg.setWindowTitle(tr("Help Improve ARLD"));
    dlg.setText(tr("Would you like to send anonymous usage statistics to help "
                   "improve ARLD?\n\nNo personal data, file contents, or network "
                   "connections are involved — usage counts are stored locally "
                   "only. You can change this setting in Help → Preferences."));
    dlg.setIcon(QMessageBox::Question);

    QPushButton* yesBtn   = dlg.addButton(tr("Yes, help improve ARLD"),
                                           QMessageBox::AcceptRole);
    QPushButton* noBtn    = dlg.addButton(tr("No thanks"),
                                           QMessageBox::RejectRole);
    /*QPushButton* laterBtn =*/ dlg.addButton(tr("Ask me later"),
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
    const QString key = QStringLiteral("telemetry/events/")
                        + QMetaEnum::fromType<Event>().valueToKey(static_cast<int>(e));
    s.setValue(key, s.value(key, 0).toInt() + 1);
}

QString TelemetryManager::stats() const {
    if (!isConsented())
        return tr("Usage statistics: not collecting (opt-in disabled).");
    QSettings s(QStringLiteral("OpsNormal"), QStringLiteral("ARLD"));
    const auto meta = QMetaEnum::fromType<Event>();
    QStringList lines;
    for (int i = 0; i < meta.keyCount(); ++i) {
        const QString key = QStringLiteral("telemetry/events/") + meta.key(i);
        const int count = s.value(key, 0).toInt();
        if (count > 0)
            lines << QStringLiteral("%1: %2").arg(meta.key(i)).arg(count);
    }
    return lines.isEmpty()
        ? tr("No events recorded yet.")
        : lines.join(QStringLiteral("\n"));
}

} // namespace arld::app
