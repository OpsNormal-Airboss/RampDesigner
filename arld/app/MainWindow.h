#pragma once
#include <QMainWindow>

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

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupLibraryPanel();
    void updateUndoRedoActions();
    void updateScaleLabel(double denominator);

    arld::ui::RampScene* m_scene;
    arld::ui::RampView* m_view;
    arld::ui::LibraryPanel* m_libraryPanel = nullptr;
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    QAction* m_drawBoundaryAction = nullptr;
    QLabel* m_scaleLabel = nullptr;
};
