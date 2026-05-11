#pragma once
#include <QMainWindow>
#include <QString>
#include <QTimer>

class QAction;
class QLabel;

namespace arld::ui {
class LibraryPanel;
class RampScene;
class RampView;
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

    arld::ui::RampScene* m_scene;
    arld::ui::RampView*  m_view;
    arld::ui::LibraryPanel* m_libraryPanel = nullptr;
    QAction* m_undoAction          = nullptr;
    QAction* m_redoAction          = nullptr;
    QAction* m_drawBoundaryAction  = nullptr;
    QAction* m_metricAction        = nullptr;
    QLabel*  m_scaleLabel          = nullptr;
    QLabel*  m_unitLabel           = nullptr;
    QLabel*  m_violationLabel      = nullptr;

    QTimer   m_autoSaveTimer;
    QString  m_autoSavePath;

    QString m_currentFilePath;
    bool    m_dirty = false;
};
