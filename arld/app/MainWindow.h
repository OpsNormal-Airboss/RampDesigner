#pragma once
#include <QMainWindow>
#include <QString>
#include <QTimer>

class QAction;
class QLabel;
class QMenu;

namespace arld::ui {
class LibraryPanel;
class PropertiesPanel;
class RampScene;
class RampView;
class ViolationsPanel;
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
    void updateWindowTitle();

    // File menu slots
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportSvg();

    // View menu slots (1-3-8)
    void fitToWindow();

    // Recent files (1-3-8)
    void addToRecentFiles(const QString& path);
    void updateRecentFilesMenu();

    arld::ui::RampScene*    m_scene;
    arld::ui::RampView*     m_view;
    arld::ui::LibraryPanel*    m_libraryPanel   = nullptr;
    arld::ui::PropertiesPanel* m_propertiesPanel = nullptr;
    arld::ui::ViolationsPanel* m_violationsPanel = nullptr;
    QAction* m_undoAction          = nullptr;
    QAction* m_redoAction          = nullptr;
    QAction* m_drawBoundaryAction  = nullptr;
    QAction* m_metricAction        = nullptr;
    QLabel*  m_scaleLabel          = nullptr;
    QLabel*  m_unitLabel           = nullptr;
    QLabel*  m_violationLabel      = nullptr;
    QMenu*   m_recentFilesMenu     = nullptr;

    QTimer   m_autoSaveTimer;
    QString  m_autoSavePath;

    QString m_currentFilePath;
    bool    m_dirty = false;
};
