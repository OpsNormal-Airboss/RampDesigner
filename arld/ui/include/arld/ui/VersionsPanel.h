#pragma once
#include <arld/core/ProjectFile.h>
#include <QDockWidget>
#include <vector>

class QListWidget;
class QPushButton;

namespace arld::ui {

class VersionsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit VersionsPanel(QWidget* parent = nullptr);

    void setProjectData(const arld::core::ProjectData& data);
    const std::vector<arld::core::LayoutVersion>& versions() const { return m_versions; }

signals:
    void versionSaveRequested(const QString& name);
    void versionSwitchRequested(const QString& id);
    void versionExportRequested(const QString& id);
    void versionDeltaRequested(const QString& idA, const QString& idB);

private slots:
    void onSaveClicked();
    void onSwitchClicked();
    void onExportClicked();
    void onDeltaClicked();
    void onSelectionChanged();

private:
    QListWidget*  m_list          = nullptr;
    QPushButton*  m_saveBtn       = nullptr;
    QPushButton*  m_switchBtn     = nullptr;
    QPushButton*  m_exportBtn     = nullptr;
    QPushButton*  m_deltaBtn      = nullptr;

    std::vector<arld::core::LayoutVersion> m_versions;
};

} // namespace arld::ui
