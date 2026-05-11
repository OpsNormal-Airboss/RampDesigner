#include "MainWindow.h"
#include <arld/ui/LibraryPanel.h>
#include <arld/ui/RampScene.h>
#include <arld/ui/RampView.h>
#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

using arld::ui::EditMode;
using arld::ui::LibraryPanel;
using arld::ui::RampScene;
using arld::ui::RampView;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("Airshow Ramp Layout Designer"));
    resize(1440, 900);

    m_scene = new RampScene(this);
    m_view  = new RampView(this);
    m_view->setRampScene(m_scene);
    setCentralWidget(m_view);

    m_scene->undoStack().onChanged = [this] { updateUndoRedoActions(); };

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupLibraryPanel();
    updateUndoRedoActions();
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), this, [] {}, QKeySequence::New);

    auto* editMenu = menuBar()->addMenu(tr("&Edit"));

    m_undoAction = editMenu->addAction(tr("&Undo"), this, [this] {
        m_scene->undoStack().undo();
    }, QKeySequence::Undo);

    m_redoAction = editMenu->addAction(tr("&Redo"), this, [this] {
        m_scene->undoStack().redo();
    }, QKeySequence::Redo);

    auto* drawMenu = menuBar()->addMenu(tr("&Draw"));
    m_drawBoundaryAction = drawMenu->addAction(tr("Draw &Boundary"), this, [this] {
        m_scene->setEditMode(
            m_scene->editMode() == EditMode::DrawBoundary
                ? EditMode::Select
                : EditMode::DrawBoundary);
    }, QKeySequence(Qt::Key_B));
    m_drawBoundaryAction->setCheckable(true);

    connect(m_scene, &RampScene::editModeChanged, this, [this](EditMode mode) {
        m_drawBoundaryAction->setChecked(mode == EditMode::DrawBoundary);
    });
}

void MainWindow::setupToolBar() {
    auto* tb = addToolBar(tr("Main"));
    tb->setMovable(false);
    tb->addAction(m_undoAction);
    tb->addAction(m_redoAction);
    tb->addSeparator();
    tb->addAction(m_drawBoundaryAction);
}

void MainWindow::setupStatusBar() {
    m_violationLabel = new QLabel(this);
    m_violationLabel->setMinimumWidth(160);
    statusBar()->addWidget(m_violationLabel);
    updateViolationLabel(0);

    m_scaleLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_scaleLabel);
    updateScaleLabel(m_view->scaleDenominator());

    connect(m_view,  &RampView::scaleChanged,         this, &MainWindow::updateScaleLabel);
    connect(m_scene, &RampScene::violationCountChanged, this, &MainWindow::updateViolationLabel);
}

void MainWindow::updateUndoRedoActions() {
    auto& stack = m_scene->undoStack();
    if (m_undoAction) {
        m_undoAction->setEnabled(stack.canUndo());
        m_undoAction->setText(stack.canUndo()
            ? tr("&Undo %1").arg(QString::fromStdString(stack.undoText()))
            : tr("&Undo"));
    }
    if (m_redoAction) {
        m_redoAction->setEnabled(stack.canRedo());
        m_redoAction->setText(stack.canRedo()
            ? tr("&Redo %1").arg(QString::fromStdString(stack.redoText()))
            : tr("&Redo"));
    }
}

void MainWindow::setupLibraryPanel() {
    m_libraryPanel = new LibraryPanel(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_libraryPanel);

    connect(m_view, &RampView::aircraftDropped,
            this, [this](const QString& id, QPointF scenePos) {
        const auto* entry = m_libraryPanel->entryById(id.toStdString());
        if (entry) m_scene->placeAircraft(*entry, scenePos);
    });
}

void MainWindow::updateScaleLabel(double denominator) {
    m_scaleLabel->setText(tr("Scale  1:%1").arg(static_cast<int>(denominator)));
}

void MainWindow::updateViolationLabel(int count) {
    if (count == 0) {
        m_violationLabel->setText(tr("✓ No clearance violations"));
        m_violationLabel->setStyleSheet("color: #00AA33; font-weight: bold;");
    } else {
        m_violationLabel->setText(tr("⚠ %1 clearance violation%2")
            .arg(count).arg(count == 1 ? "" : "s"));
        m_violationLabel->setStyleSheet("color: #CC2222; font-weight: bold;");
    }
}
