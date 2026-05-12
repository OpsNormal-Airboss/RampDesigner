#pragma once
#include <arld/core/ProjectFile.h>
#include <QMainWindow>
#include <QString>
#include <QTimer>

class QAction;
class QDockWidget;
class QLabel;
class QMenu;
class QSlider;

namespace arld::ui {
class LibraryPanel;
class MinimapWidget;
class PropertiesPanel;
class RampScene;
class RampView;
class UndoHistoryPanel;
class ViolationsPanel;
class VersionsPanel;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void autoSave();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupLibraryPanel();
    void setupPanels();
    void setupFileActions();
    void updateUndoRedoActions();
    void updateScaleLabel(double denominator);
    void updateViolationLabel(int count);
    void updateAircraftCountLabel(int count);
    void updateWindowTitle();

    // File menu slots
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportSvg();
    void exportPdf();
    void exportViolationsReport();
    void exportAll();
    void exportAircraftManifest();
    void exportPng();
    void exportJpeg();
    void editProjectMetadata();

    // View menu slots (1-3-8)
    void fitToWindow();

    // Recent files (1-3-8)
    void addToRecentFiles(const QString& path);
    void updateRecentFilesMenu();

    // Sprint 2-2 slots
    void importBoundary();
    void showSatelliteTilesDialog();

    // Sprint 2-3 slots
    void showAboutDialog();
    void showLibraryUpdateDialog();
    void showUserManual();

    void saveLayout();
    void restoreLayout();
    void closeEvent(QCloseEvent* event) override;

    arld::ui::RampScene*    m_scene;
    arld::ui::RampView*     m_view;
    arld::ui::LibraryPanel*       m_libraryPanel    = nullptr;
    arld::ui::MinimapWidget*      m_minimapWidget   = nullptr;
    arld::ui::PropertiesPanel*    m_propertiesPanel = nullptr;
    arld::ui::UndoHistoryPanel*   m_undoHistoryPanel = nullptr;
    arld::ui::VersionsPanel*      m_versionsPanel   = nullptr;
    arld::ui::ViolationsPanel*    m_violationsPanel = nullptr;
    QDockWidget*                  m_minimapDock     = nullptr;

    // Sprint 2-2: current project data (holds versions list)
    arld::core::ProjectData m_currentData;
    QAction* m_undoAction          = nullptr;
    QAction* m_redoAction          = nullptr;
    QAction* m_deleteAction        = nullptr;
    QAction* m_drawBoundaryAction  = nullptr;
    QAction* m_metricAction        = nullptr;
    QLabel*  m_scaleLabel          = nullptr;
    QLabel*  m_unitLabel           = nullptr;
    QLabel*  m_violationLabel      = nullptr;
    QLabel*  m_aircraftCountLabel  = nullptr;
    QMenu*   m_recentFilesMenu     = nullptr;
    QLabel*  m_rulesetLabel        = nullptr;

    QTimer   m_autoSaveTimer;
    QString  m_autoSavePath;

    QString m_currentFilePath;
    bool    m_dirty         = false;
    bool    m_suppressDirty = false; // suppresses changed-signal during load/clear

    // Project metadata (show-specific fields stored in MainWindow, not RampScene).
    arld::core::ProjectMetadata m_projectMetadata;
};
