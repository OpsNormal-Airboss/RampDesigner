#include "MainWindow.h"
#include <arld/ui/ClearanceRuleDialog.h>
#include <arld/ui/LibraryPanel.h>
#include <arld/ui/PropertiesPanel.h>
#include <arld/ui/RampScene.h>
#include <arld/ui/RampView.h>
#include <arld/ui/ViolationsPanel.h>
#include <arld/ui/AircraftItem.h>
#include <arld/core/ProjectFile.h>
#include <arld/core/UnitConverter.h>
#include <arld/export/SvgExporter.h>
#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>

using arld::ui::EditMode;
using arld::ui::LibraryPanel;
using arld::ui::PropertiesPanel;
using arld::ui::RampScene;
using arld::ui::RampView;
using arld::ui::ViolationsPanel;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("Airshow Ramp Layout Designer"));
    resize(1440, 900);

    m_scene = new RampScene(this);
    m_view  = new RampView(this);
    m_view->setRampScene(m_scene);
    setCentralWidget(m_view);

    m_scene->undoStack().onChanged = [this] { updateUndoRedoActions(); };

    // Mark scene dirty whenever it changes.
    connect(m_scene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
        if (!m_dirty) {
            m_dirty = true;
            updateWindowTitle();
        }
    });

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupLibraryPanel();
    setupPanels();
    updateUndoRedoActions();
    updateWindowTitle();

    // --- Story 1-1-4: Auto-save ---
    m_autoSavePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                     + "/arld_autosave.arld";
    QDir().mkpath(QFileInfo(m_autoSavePath).absolutePath());
    m_autoSaveTimer.setInterval(60000);
    m_autoSaveTimer.start();
    connect(&m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);

    // --- Story 1-1-5: Crash recovery dialog ---
    if (QFile::exists(m_autoSavePath)) {
        const auto btn = QMessageBox::question(
            this, tr("Recover Unsaved Work"),
            tr("An auto-saved layout was found from a previous session.\nRestore it?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (btn == QMessageBox::Yes) {
            try {
                auto data = arld::core::ProjectFile::load(m_autoSavePath.toStdString());
                auto lookup = [this](const std::string& id) -> const arld::core::AircraftLibraryEntry* {
                    return m_libraryPanel->entryById(id);
                };
                m_scene->loadProjectData(data, lookup);
                m_dirty = true;
                updateWindowTitle();
                updateUndoRedoActions();
            } catch (...) {
                QMessageBox::warning(this, tr("Recovery Failed"),
                                     tr("Could not restore the auto-saved layout."));
            }
        }
        QFile::remove(m_autoSavePath); // always delete after attempting recovery
    }
}

MainWindow::~MainWindow() {
    QFile::remove(m_autoSavePath);
}

void MainWindow::setupMenuBar() {
    // ---- File menu ----
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    setupFileActions();
    (void)fileMenu; // actions added via setupFileActions

    // ---- Edit menu ----
    auto* editMenu = menuBar()->addMenu(tr("&Edit"));

    m_undoAction = editMenu->addAction(tr("&Undo"), this, [this] {
        m_scene->undoStack().undo();
    }, QKeySequence::Undo);

    m_redoAction = editMenu->addAction(tr("&Redo"), this, [this] {
        m_scene->undoStack().redo();
    }, QKeySequence::Redo);

    // ---- Draw menu ----
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

    // ---- Tools menu ----
    auto* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Clearance &Rules..."), this, [this] {
        arld::ui::ClearanceRuleDialog dlg(m_scene->ruleSet(), this);
        if (dlg.exec() == QDialog::Accepted) {
            m_scene->setRuleSet(dlg.ruleSet());
        }
    });

    // ---- View menu ----
    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    m_metricAction = viewMenu->addAction(tr("Show in &Metric"), this, [this](bool checked) {
        arld::core::UnitConverter::instance().setUnitSystem(
            checked ? arld::core::UnitSystem::Metric : arld::core::UnitSystem::Imperial);
        updateScaleLabel(m_view->scaleDenominator());
        if (m_unitLabel) {
            m_unitLabel->setText(tr("Units: %1")
                .arg(arld::core::UnitConverter::instance().suffix()));
        }
    }, QKeySequence(Qt::Key_M));
    m_metricAction->setCheckable(true);
}

void MainWindow::setupFileActions() {
    auto* fileMenu = menuBar()->findChild<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
    // Re-grab the File menu by its title (it was just added).
    QMenu* fm = nullptr;
    for (auto* action : menuBar()->actions()) {
        if (action->menu() && action->text() == tr("&File")) {
            fm = action->menu();
            break;
        }
    }
    if (!fm) return;

    fm->addAction(tr("&New"),  this, &MainWindow::newProject,   QKeySequence::New);
    fm->addAction(tr("&Open..."), this, &MainWindow::openProject, QKeySequence::Open);
    fm->addSeparator();
    fm->addAction(tr("&Save"),      this, &MainWindow::saveProject,   QKeySequence::Save);
    fm->addAction(tr("Save &As..."), this, &MainWindow::saveProjectAs, QKeySequence::SaveAs);
    fm->addSeparator();
    fm->addAction(tr("Export &SVG..."), this, &MainWindow::exportSvg);
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

    m_unitLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_unitLabel);
    m_unitLabel->setText(tr("Units: %1").arg(arld::core::UnitConverter::instance().suffix()));

    m_scaleLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_scaleLabel);
    updateScaleLabel(m_view->scaleDenominator());

    connect(m_view,  &RampView::scaleChanged,           this, &MainWindow::updateScaleLabel);
    connect(m_scene, &RampScene::violationCountChanged,  this, &MainWindow::updateViolationLabel);
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

void MainWindow::setupPanels() {
    // Properties panel (right dock)
    m_propertiesPanel = new PropertiesPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesPanel);

    // Wire selection changes to properties panel
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this] {
        arld::ui::AircraftItem* selected = nullptr;
        const auto items = m_scene->selectedItems();
        for (auto* item : items) {
            if (auto* ac = qgraphicsitem_cast<arld::ui::AircraftItem*>(item)) {
                selected = ac;
                break;
            }
        }
        m_propertiesPanel->setAircraft(selected);
    });

    // Violations panel (bottom dock)
    m_violationsPanel = new ViolationsPanel(m_scene, m_view, this);
    addDockWidget(Qt::BottomDockWidgetArea, m_violationsPanel);

    connect(m_scene, &RampScene::violationsChanged, this, [this] {
        m_violationsPanel->refresh(m_scene->lastViolations());
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

void MainWindow::updateWindowTitle() {
    const QString base = m_currentFilePath.isEmpty()
        ? tr("Untitled")
        : QFileInfo(m_currentFilePath).fileName();
    setWindowTitle(tr("%1%2 — Airshow Ramp Layout Designer")
                       .arg(base)
                       .arg(m_dirty ? "*" : ""));
}

// ---------------------------------------------------------------------------
// Auto-save
// ---------------------------------------------------------------------------
void MainWindow::autoSave() {
    if (!m_dirty) {
        return;
    }
    try {
        auto data = m_scene->toProjectData();
        arld::core::ProjectFile::save(m_autoSavePath.toStdString(), data);
    } catch (...) {
        // Silently swallow errors — auto-save is best-effort.
    }
}

// ---------------------------------------------------------------------------
// File operations
// ---------------------------------------------------------------------------
void MainWindow::newProject() {
    if (m_dirty) {
        const auto btn = QMessageBox::question(
            this, tr("New Project"),
            tr("The current layout has unsaved changes. Discard them?"),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (btn != QMessageBox::Discard) return;
    }
    m_scene->clearScene();
    m_currentFilePath.clear();
    m_dirty = false;
    updateWindowTitle();
    updateUndoRedoActions();
    if (m_violationsPanel) m_violationsPanel->refresh({});
}

void MainWindow::openProject() {
    if (m_dirty) {
        const auto btn = QMessageBox::question(
            this, tr("Open Project"),
            tr("The current layout has unsaved changes. Discard them?"),
            QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (btn != QMessageBox::Discard) return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open ARLD Project"), QString(),
        tr("ARLD Project Files (*.arld);;All Files (*)"));
    if (path.isEmpty()) return;

    try {
        auto data = arld::core::ProjectFile::load(path.toStdString());

        auto lookup = [this](const std::string& id) -> const arld::core::AircraftLibraryEntry* {
            return m_libraryPanel->entryById(id);
        };
        m_scene->loadProjectData(data, lookup);

        m_currentFilePath = path;
        m_dirty = false;
        updateWindowTitle();
        updateUndoRedoActions();
        if (m_violationsPanel) m_violationsPanel->refresh(m_scene->lastViolations());
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Open Failed"),
                              tr("Could not open project:\n%1").arg(e.what()));
    }
}

void MainWindow::saveProject() {
    if (m_currentFilePath.isEmpty()) {
        saveProjectAs();
        return;
    }
    try {
        auto data = m_scene->toProjectData();
        if (data.metadata.title == "Untitled Layout" && !m_currentFilePath.isEmpty())
            data.metadata.title = QFileInfo(m_currentFilePath).baseName().toStdString();
        if (data.metadata.createdUtc.empty())
            data.metadata.createdUtc = arld::core::ProjectFile::currentUtcTimestamp();
        arld::core::ProjectFile::save(m_currentFilePath.toStdString(), data);
        m_dirty = false;
        updateWindowTitle();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Save Failed"),
                              tr("Could not save project:\n%1").arg(e.what()));
    }
}

void MainWindow::saveProjectAs() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save ARLD Project As"), m_currentFilePath,
        tr("ARLD Project Files (*.arld);;All Files (*)"));
    if (path.isEmpty()) return;

    m_currentFilePath = path;
    saveProject();
}

void MainWindow::exportSvg() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export SVG"), QString(),
        tr("SVG Files (*.svg);;All Files (*)"));
    if (path.isEmpty()) return;

    try {
        auto data = m_scene->toProjectData();
        arld::export_::SvgExporter exporter;
        exporter.exportLayout(data, path.toStdString());
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Could not export SVG:\n%1").arg(e.what()));
    }
}
