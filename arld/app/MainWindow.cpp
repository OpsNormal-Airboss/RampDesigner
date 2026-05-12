#include "MainWindow.h"
#include "AppBadge.h"
#include <arld/ui/ClearanceRuleDialog.h>
#include <arld/ui/LibraryPanel.h>
#include <arld/ui/LibraryUpdateChecker.h>
#include <arld/ui/MinimapWidget.h>
#include <arld/ui/PropertiesPanel.h>
#include <arld/ui/RampScene.h>
#include <arld/ui/RampView.h>
#include <arld/ui/SatelliteUnderlayItem.h>
#include <arld/ui/UndoHistoryPanel.h>
#include <arld/ui/VersionsPanel.h>
#include <arld/ui/ViolationsPanel.h>
#include <arld/ui/AircraftItem.h>
#include <arld/core/BoundaryImporter.h>
#include <arld/core/Config.h>
#include <arld/core/LayoutDiffer.h>
#include <arld/core/ProjectFile.h>
#include <arld/core/UnitConverter.h>
#include <arld/export/AircraftManifestExporter.h>
#include <arld/export/BatchExporter.h>
#include <arld/export/PdfExporter.h>
#include <arld/export/SvgExporter.h>
#include <arld/export/ViolationReportExporter.h>
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringList>
#include <QTextBrowser>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidgetAction>

using arld::ui::EditMode;
using arld::ui::LibraryPanel;
using arld::ui::MinimapWidget;
using arld::ui::PropertiesPanel;
using arld::ui::RampScene;
using arld::ui::RampView;
using arld::ui::UndoHistoryPanel;
using arld::ui::VersionsPanel;
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

    // Keyboard navigation (1-4-8): Tab order follows visual left→right/top→bottom layout.
    if (m_libraryPanel->focusProxy() && m_propertiesPanel->focusProxy())
        QWidget::setTabOrder(m_libraryPanel->focusProxy(), m_propertiesPanel->focusProxy());
    if (m_propertiesPanel->focusProxy() && m_violationsPanel->focusProxy())
        QWidget::setTabOrder(m_propertiesPanel->focusProxy(), m_violationsPanel->focusProxy());

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
                m_projectMetadata = data.metadata;
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

    // Fit to Window (1-3-8)
    viewMenu->addAction(tr("&Fit to Window"), this, &MainWindow::fitToWindow,
                        QKeySequence(Qt::CTRL | Qt::Key_0));

    // Grid controls (1-3-7)
    viewMenu->addSeparator();

    auto* showGridAction = viewMenu->addAction(tr("Show &Grid"), this, [this](bool checked) {
        m_scene->setGridVisible(checked);
    }, QKeySequence(Qt::Key_G));
    showGridAction->setCheckable(true);
    showGridAction->setChecked(true);

    auto* spacingMenu = viewMenu->addMenu(tr("Grid &Spacing"));
    auto* spacingGroup = new QActionGroup(this);
    spacingGroup->setExclusive(true);

    auto addSpacing = [&](const QString& label, int ft) {
        auto* act = spacingMenu->addAction(label, this, [this, ft] {
            m_scene->setGridSpacingFt(ft);
        });
        act->setCheckable(true);
        spacingGroup->addAction(act);
        if (ft == 50) act->setChecked(true); // default
    };
    addSpacing(tr("25 ft"),  25);
    addSpacing(tr("50 ft"),  50);
    addSpacing(tr("100 ft"), 100);

    // Satellite underlay controls (1-5-6 / 2-2)
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Satellite &Tiles..."), this, &MainWindow::showSatelliteTilesDialog);
    viewMenu->addAction(tr("Load &Satellite Image..."), this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Load Satellite Image"), QString(),
            tr("Image Files (*.png *.jpg *.jpeg *.tif *.tiff *.bmp);;All Files (*)"));
        if (!path.isEmpty())
            m_scene->setSatelliteImage(path);
    });

    // Opacity slider in a widget action
    auto* opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setMinimum(0);
    opacitySlider->setMaximum(100);
    opacitySlider->setValue(80);
    opacitySlider->setMinimumWidth(120);
    connect(opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_scene->setSatelliteOpacity(value / 100.0f);
    });

    auto* opacityAction = new QWidgetAction(this);
    auto* sliderWidget = new QWidget();
    auto* sliderLayout = new QHBoxLayout(sliderWidget);
    sliderLayout->setContentsMargins(8, 2, 8, 2);
    sliderLayout->addWidget(new QLabel(tr("Satellite Opacity:")));
    sliderLayout->addWidget(opacitySlider);
    opacityAction->setDefaultWidget(sliderWidget);
    viewMenu->addAction(opacityAction);

    // ---- Help menu ----
    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("Check for Library &Updates..."), this,
                        &MainWindow::showLibraryUpdateDialog);
    helpMenu->addSeparator();
    helpMenu->addAction(tr("About &ARLD..."), this, &MainWindow::showAboutDialog);
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
    fm->addAction(tr("Project &Metadata..."), this, &MainWindow::editProjectMetadata);
    fm->addSeparator();
    fm->addAction(tr("Export &SVG..."), this, &MainWindow::exportSvg);
    fm->addAction(tr("Export &PDF..."), this, &MainWindow::exportPdf);
    fm->addAction(tr("Export &All Formats..."), this, &MainWindow::exportAll);
    fm->addSeparator();
    fm->addAction(tr("Export &Violations Report..."), this, &MainWindow::exportViolationsReport);
    fm->addAction(tr("Export Aircraft &Manifest CSV..."), this, &MainWindow::exportAircraftManifest);
    fm->addSeparator();
    fm->addAction(tr("Import &Boundary from KML/GeoJSON..."), this, &MainWindow::importBoundary);
    fm->addSeparator();
    m_recentFilesMenu = fm->addMenu(tr("&Recent Projects"));
    updateRecentFilesMenu();
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

    m_aircraftCountLabel = new QLabel(this);
    m_aircraftCountLabel->setMinimumWidth(120);
    statusBar()->addWidget(m_aircraftCountLabel);
    updateAircraftCountLabel(0);

    m_unitLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_unitLabel);
    m_unitLabel->setText(tr("Units: %1").arg(arld::core::UnitConverter::instance().suffix()));

    m_scaleLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_scaleLabel);
    updateScaleLabel(m_view->scaleDenominator());

    connect(m_view,  &RampView::scaleChanged,            this, &MainWindow::updateScaleLabel);
    connect(m_scene, &RampScene::violationCountChanged,  this, &MainWindow::updateViolationLabel);
    connect(m_scene, &RampScene::aircraftCountChanged,   this, &MainWindow::updateAircraftCountLabel);
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
    m_libraryPanel->setAccessibleName(tr("Aircraft Library panel"));

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
    m_propertiesPanel->setAccessibleName(tr("Aircraft Properties panel"));

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

    // Wire "Properties..." right-click to raise the panel
    connect(m_scene, &RampScene::propertiesRequested,
            this, [this](arld::ui::AircraftItem* item) {
        m_propertiesPanel->setAircraft(item);
        m_propertiesPanel->setVisible(true);
        m_propertiesPanel->raise();
    });

    // Violations panel (bottom dock)
    m_violationsPanel = new ViolationsPanel(m_scene, m_view, this);
    addDockWidget(Qt::BottomDockWidgetArea, m_violationsPanel);
    m_violationsPanel->setAccessibleName(tr("Clearance Violations panel"));

    connect(m_scene, &RampScene::violationsChanged, this, [this] {
        m_violationsPanel->refresh(m_scene->lastViolations());
    });

    // --- Sprint 2-2 panels ---

    // Versions panel (right dock, tabified with properties)
    m_versionsPanel = new VersionsPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_versionsPanel);
    tabifyDockWidget(m_propertiesPanel, m_versionsPanel);

    connect(m_versionsPanel, &VersionsPanel::versionSaveRequested,
            this, [this](const QString& name) {
        // Snapshot current scene into a LayoutVersion and push onto m_currentData.versions
        auto snapshot = m_scene->toProjectData();
        arld::core::LayoutVersion ver;
        ver.id         = arld::core::ProjectFile::generateUuid();
        ver.name       = name.toStdString();
        ver.createdUtc = arld::core::ProjectFile::currentUtcTimestamp();
        ver.boundary   = snapshot.boundary;
        ver.aircraft   = snapshot.aircraft;
        m_currentData.versions.push_back(ver);
        m_versionsPanel->setProjectData(m_currentData);
        m_dirty = true;
        updateWindowTitle();
    });

    connect(m_versionsPanel, &VersionsPanel::versionSwitchRequested,
            this, [this](const QString& id) {
        const std::string sid = id.toStdString();
        for (const auto& ver : m_currentData.versions) {
            if (ver.id == sid) {
                arld::core::ProjectData vdata;
                vdata.boundary = ver.boundary;
                vdata.aircraft = ver.aircraft;
                auto lookup = [this](const std::string& libId)
                    -> const arld::core::AircraftLibraryEntry* {
                    return m_libraryPanel->entryById(libId);
                };
                m_scene->clearDelta();
                m_scene->loadProjectData(vdata, lookup);
                m_dirty = true;
                updateWindowTitle();
                return;
            }
        }
    });

    connect(m_versionsPanel, &VersionsPanel::versionExportRequested,
            this, [this](const QString& id) {
        const std::string sid = id.toStdString();
        for (const auto& ver : m_currentData.versions) {
            if (ver.id == sid) {
                const QString path = QFileDialog::getSaveFileName(
                    this, tr("Export Layout Version"), QString(),
                    tr("ARLD Project Files (*.arld);;All Files (*)"));
                if (path.isEmpty()) return;
                arld::core::ProjectData vdata;
                vdata.metadata = m_projectMetadata;
                vdata.metadata.title = ver.name;
                vdata.boundary = ver.boundary;
                vdata.aircraft = ver.aircraft;
                try {
                    arld::core::ProjectFile::save(path.toStdString(), vdata);
                } catch (const std::exception& e) {
                    QMessageBox::critical(this, tr("Export Failed"),
                                          tr("Could not export version:\n%1").arg(e.what()));
                }
                return;
            }
        }
    });

    connect(m_versionsPanel, &VersionsPanel::versionDeltaRequested,
            this, [this](const QString& idA, const QString& idB) {
        const std::string sidA = idA.toStdString();
        const std::string sidB = idB.toStdString();
        const arld::core::LayoutVersion* verA = nullptr;
        const arld::core::LayoutVersion* verB = nullptr;
        for (const auto& ver : m_currentData.versions) {
            if (ver.id == sidA) verA = &ver;
            if (ver.id == sidB) verB = &ver;
        }
        if (!verA || !verB) return;
        const auto delta = arld::core::LayoutDiffer::diff(verA->aircraft, verB->aircraft);
        m_scene->showDelta(delta);
    });

    // Undo history panel (right dock, tabified with versions)
    m_undoHistoryPanel = new UndoHistoryPanel(m_scene, this);
    addDockWidget(Qt::RightDockWidgetArea, m_undoHistoryPanel);
    tabifyDockWidget(m_versionsPanel, m_undoHistoryPanel);

    // Minimap widget (bottom dock, tabified with violations)
    auto* minimapDock = new QDockWidget(tr("Minimap"), this);
    minimapDock->setObjectName(QStringLiteral("MinimapDock"));
    m_minimapWidget = new MinimapWidget(this);
    m_minimapWidget->setScene(m_scene);
    m_minimapWidget->setView(m_view);
    minimapDock->setWidget(m_minimapWidget);
    addDockWidget(Qt::BottomDockWidgetArea, minimapDock);
    tabifyDockWidget(m_violationsPanel, minimapDock);

    connect(m_minimapWidget, &MinimapWidget::minimapClicked,
            this, [this](QPointF scenePos) {
        m_view->centerOn(scenePos);
    });
}

void MainWindow::updateScaleLabel(double denominator) {
    m_scaleLabel->setText(tr("Scale  1:%1").arg(static_cast<int>(denominator)));
}

void MainWindow::updateViolationLabel(int count) {
    AppBadge::setCount(count);
    if (count == 0) {
        m_violationLabel->setText(tr("✓ No clearance violations"));
        m_violationLabel->setStyleSheet("color: #00AA33; font-weight: bold;");
    } else {
        m_violationLabel->setText(tr("⚠ %1 clearance violation%2")
            .arg(count).arg(count == 1 ? "" : "s"));
        m_violationLabel->setStyleSheet("color: #CC2222; font-weight: bold;");
    }
}

void MainWindow::updateAircraftCountLabel(int count) {
    m_aircraftCountLabel->setText(tr("%1 aircraft").arg(count));
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
        data.metadata = m_projectMetadata;
        data.metadata.modifiedUtc = arld::core::ProjectFile::currentUtcTimestamp();
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
    m_projectMetadata = arld::core::ProjectMetadata{};
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
        m_projectMetadata = data.metadata;
        m_currentData = data;
        if (m_versionsPanel) m_versionsPanel->setProjectData(m_currentData);

        m_currentFilePath = path;
        m_dirty = false;
        updateWindowTitle();
        updateUndoRedoActions();
        addToRecentFiles(path);
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
        data.metadata = m_projectMetadata;
        if (data.metadata.title == "Untitled Layout" && !m_currentFilePath.isEmpty())
            data.metadata.title = QFileInfo(m_currentFilePath).baseName().toStdString();
        if (data.metadata.createdUtc.empty())
            data.metadata.createdUtc = arld::core::ProjectFile::currentUtcTimestamp();
        // Preserve named versions
        data.versions = m_currentData.versions;
        arld::core::ProjectFile::save(m_currentFilePath.toStdString(), data);
        m_currentData = data;
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
    addToRecentFiles(path);
}

void MainWindow::exportSvg() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export SVG"), QString(),
        tr("SVG Files (*.svg);;All Files (*)"));
    if (path.isEmpty()) return;

    try {
        auto data = m_scene->toProjectData();
        data.metadata = m_projectMetadata;
        arld::export_::SvgExporter exporter;
        exporter.exportLayout(data, path.toStdString());
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Could not export SVG:\n%1").arg(e.what()));
    }
}

void MainWindow::exportPdf() {
    // --- Paper size / orientation dialog ---
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Export PDF"));
    auto* layout = new QFormLayout(&dlg);

    auto* sizeCombo = new QComboBox();
    sizeCombo->addItems({tr("Letter"), tr("Tabloid"), tr("ANSI-C"), tr("ANSI-D"),
                         tr("ANSI-E"), tr("ANSI-E1")});
    sizeCombo->setCurrentIndex(3); // ANSI-D default
    layout->addRow(tr("Paper Size:"), sizeCombo);

    auto* portraitRadio  = new QRadioButton(tr("Portrait"));
    auto* landscapeRadio = new QRadioButton(tr("Landscape"));
    landscapeRadio->setChecked(true);
    auto* orientGroup = new QWidget();
    auto* orientLayout = new QHBoxLayout(orientGroup);
    orientLayout->setContentsMargins(0, 0, 0, 0);
    orientLayout->addWidget(portraitRadio);
    orientLayout->addWidget(landscapeRadio);
    layout->addRow(tr("Orientation:"), orientGroup);

    auto* incViolCheck = new QCheckBox(tr("Include Violations Report page"));
    layout->addRow(incViolCheck);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export PDF"), QString(),
        tr("PDF Files (*.pdf);;All Files (*)"));
    if (path.isEmpty()) return;

    try {
        auto data = m_scene->toProjectData();
        data.metadata = m_projectMetadata;

        static const arld::export_::ExportOptions::PaperSize sizes[] = {
            arld::export_::ExportOptions::PaperSize::Letter,
            arld::export_::ExportOptions::PaperSize::Tabloid,
            arld::export_::ExportOptions::PaperSize::ANSI_C,
            arld::export_::ExportOptions::PaperSize::ANSI_D,
            arld::export_::ExportOptions::PaperSize::ANSI_E,
            arld::export_::ExportOptions::PaperSize::ANSI_E1,
        };

        arld::export_::ExportOptions opts;
        opts.paperSize   = sizes[sizeCombo->currentIndex()];
        opts.orientation = landscapeRadio->isChecked()
                           ? arld::export_::ExportOptions::Orientation::Landscape
                           : arld::export_::ExportOptions::Orientation::Portrait;
        opts.showName  = m_projectMetadata.title;
        opts.showDate  = m_projectMetadata.showDate;
        opts.showVenue = m_projectMetadata.showVenue;
        opts.arldFilePath = m_currentFilePath.toStdString();

        if (incViolCheck->isChecked()) {
            opts.includeViolations = true;
            opts.violations = m_scene->lastViolations();
            opts.overrides  = m_scene->overrides();
        }

        arld::export_::PdfExporter exporter;
        exporter.exportLayout(data, path.toStdString(), opts);

        QMessageBox::information(this, tr("Export Successful"),
                                 tr("PDF exported to:\n%1").arg(path));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Export Failed"),
                              tr("Could not export PDF:\n%1").arg(e.what()));
    }
}

void MainWindow::exportViolationsReport() {
    // Ask: PDF or CSV?
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Export Violations Report"));
    auto* layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(tr("Export format:")));
    auto* csvRadio  = new QRadioButton(tr("CSV (comma-separated)"));
    auto* pdfRadio  = new QRadioButton(tr("PDF (additional page)"));
    csvRadio->setChecked(true);
    layout->addWidget(csvRadio);
    layout->addWidget(pdfRadio);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);
    if (dlg.exec() != QDialog::Accepted) return;

    const bool exportCsv = csvRadio->isChecked();

    if (exportCsv) {
        const QString path = QFileDialog::getSaveFileName(
            this, tr("Export Violations CSV"), QString(),
            tr("CSV Files (*.csv);;All Files (*)"));
        if (path.isEmpty()) return;
        try {
            arld::export_::ViolationReportExporter::exportCsv(
                m_scene->lastViolations(),
                m_scene->overrides(),
                path.toStdString());
            QMessageBox::information(this, tr("Export Successful"),
                                     tr("Violations report exported to:\n%1").arg(path));
        } catch (const std::exception& e) {
            QMessageBox::critical(this, tr("Export Failed"),
                                  tr("Could not export CSV:\n%1").arg(e.what()));
        }
    } else {
        // PDF: export layout with violations page
        const QString path = QFileDialog::getSaveFileName(
            this, tr("Export Violations PDF"), QString(),
            tr("PDF Files (*.pdf);;All Files (*)"));
        if (path.isEmpty()) return;
        try {
            auto data = m_scene->toProjectData();
            data.metadata = m_projectMetadata;

            arld::export_::ExportOptions opts;
            opts.includeViolations = true;
            opts.violations = m_scene->lastViolations();
            opts.overrides  = m_scene->overrides();
            opts.showName   = m_projectMetadata.title;
            opts.showDate   = m_projectMetadata.showDate;
            opts.showVenue  = m_projectMetadata.showVenue;

            arld::export_::PdfExporter exporter;
            exporter.exportLayout(data, path.toStdString(), opts);
            QMessageBox::information(this, tr("Export Successful"),
                                     tr("Violations PDF exported to:\n%1").arg(path));
        } catch (const std::exception& e) {
            QMessageBox::critical(this, tr("Export Failed"),
                                  tr("Could not export PDF:\n%1").arg(e.what()));
        }
    }
}

void MainWindow::editProjectMetadata() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Project Metadata"));
    auto* layout = new QFormLayout(&dlg);

    auto* titleEdit = new QLineEdit(QString::fromStdString(m_projectMetadata.title));
    auto* dateEdit  = new QLineEdit(QString::fromStdString(m_projectMetadata.showDate));
    auto* venueEdit = new QLineEdit(QString::fromStdString(m_projectMetadata.showVenue));

    layout->addRow(tr("Show Name:"),  titleEdit);
    layout->addRow(tr("Show Date:"),  dateEdit);
    layout->addRow(tr("Show Venue:"), venueEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted) return;

    m_projectMetadata.title     = titleEdit->text().toStdString();
    m_projectMetadata.showDate  = dateEdit->text().toStdString();
    m_projectMetadata.showVenue = venueEdit->text().toStdString();
    m_dirty = true;
    updateWindowTitle();
}

// ---------------------------------------------------------------------------
// 1-3-8: Fit to Window
// ---------------------------------------------------------------------------
void MainWindow::fitToWindow() {
    const QRectF bounds = m_scene->itemsBoundingRect();
    if (bounds.isNull()) return;
    m_view->fitInView(bounds.adjusted(-50, -50, 50, 50), Qt::KeepAspectRatio);
}

// ---------------------------------------------------------------------------
// 1-3-8: Recent files
// ---------------------------------------------------------------------------
void MainWindow::addToRecentFiles(const QString& path) {
    if (path.isEmpty()) return;
    QSettings settings(QStringLiteral("OpsNormal Airboss"), QStringLiteral("ARLD"));
    QStringList recent = settings.value(QStringLiteral("recentFiles")).toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > 10)
        recent.removeLast();
    settings.setValue(QStringLiteral("recentFiles"), recent);
    updateRecentFilesMenu();
}

void MainWindow::exportAll() {
    // Strip any extension from the suggested path
    QString suggested = m_currentFilePath;
    if (!suggested.isEmpty()) {
        QFileInfo fi(suggested);
        suggested = fi.absolutePath() + "/" + fi.completeBaseName();
    }

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export All Formats"),
        suggested,
        tr("All Files (*)"));
    if (path.isEmpty()) return;

    // Strip any extension the user typed
    QString basePath = path;
    {
        QFileInfo fi(path);
        const QString suf = fi.suffix().toLower();
        if (suf == "svg" || suf == "pdf" || suf == "png" || suf == "jpg" || suf == "jpeg")
            basePath = fi.absolutePath() + "/" + fi.completeBaseName();
    }

    try {
        auto data = m_scene->toProjectData();
        data.metadata = m_projectMetadata;

        arld::export_::ExportOptions opts;
        opts.paperSize   = arld::export_::ExportOptions::PaperSize::ANSI_D;
        opts.orientation = arld::export_::ExportOptions::Orientation::Landscape;
        opts.showName    = m_projectMetadata.title;
        opts.showDate    = m_projectMetadata.showDate;
        opts.showVenue   = m_projectMetadata.showVenue;

        arld::export_::BatchExporter::exportAll(data, basePath.toStdString(), opts);

        QMessageBox::information(this, tr("Export Successful"),
            tr("Layout exported to:\n  %1.svg\n  %1.pdf\n  %1.png\n  %1.jpg")
                .arg(basePath));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Export Partially Failed"),
            tr("Some formats could not be exported:\n%1").arg(e.what()));
    }
}

void MainWindow::exportAircraftManifest() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Aircraft Manifest CSV"), QString(),
        tr("CSV Files (*.csv);;All Files (*)"));
    if (path.isEmpty()) return;

    try {
        auto data = m_scene->toProjectData();
        data.metadata = m_projectMetadata;
        arld::export_::AircraftManifestExporter::exportCsv(data, path.toStdString());
        QMessageBox::information(this, tr("Export Successful"),
            tr("Aircraft manifest exported to:\n%1").arg(path));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Could not export manifest:\n%1").arg(e.what()));
    }
}

// ---------------------------------------------------------------------------
// Sprint 2-2: Import Boundary from KML/GeoJSON
// ---------------------------------------------------------------------------
void MainWindow::importBoundary() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Import Boundary"),
        QString(),
        tr("KML/GeoJSON (*.kml *.geojson *.json);;All Files (*)"));
    if (path.isEmpty()) return;

    try {
        const auto boundary = arld::core::BoundaryImporter::importFile(path.toStdString());
        m_scene->setBoundary(boundary);
        m_dirty = true;
        updateWindowTitle();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Import Failed"),
                              tr("Could not import boundary:\n%1").arg(e.what()));
    }
}

// ---------------------------------------------------------------------------
// Sprint 2-2: Satellite Tiles dialog
// ---------------------------------------------------------------------------
void MainWindow::showSatelliteTilesDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Satellite Tiles"));
    auto* layout = new QFormLayout(&dlg);

    QSettings settings(QStringLiteral("OpsNormal"), QStringLiteral("ARLD"));
    const QString savedToken = settings.value(QStringLiteral("mapboxToken")).toString();
    const QString defaultUrl = QStringLiteral(
        "https://api.mapbox.com/styles/v1/mapbox/satellite-v9/static/"
        "{lon},{lat},{zoom}/1280x720@2x?access_token={token}");
    const QString savedUrl = settings.value(QStringLiteral("mapboxTileUrl"), defaultUrl).toString();

    auto* tokenEdit = new QLineEdit(savedToken, &dlg);
    tokenEdit->setEchoMode(QLineEdit::Password);
    tokenEdit->setPlaceholderText(tr("pk.eyJ1..."));
    layout->addRow(tr("Mapbox Access Token:"), tokenEdit);

    auto* urlEdit = new QLineEdit(savedUrl, &dlg);
    layout->addRow(tr("Tile URL Template:"), urlEdit);

    // Latitude / longitude / zoom
    auto* latSpin = new QDoubleSpinBox(&dlg);
    latSpin->setRange(-90.0, 90.0);
    latSpin->setDecimals(6);
    latSpin->setSingleStep(0.001);
    latSpin->setValue(settings.value(QStringLiteral("satLat"), 44.5).toDouble());
    layout->addRow(tr("Latitude (°):"), latSpin);

    auto* lonSpin = new QDoubleSpinBox(&dlg);
    lonSpin->setRange(-180.0, 180.0);
    lonSpin->setDecimals(6);
    lonSpin->setSingleStep(0.001);
    lonSpin->setValue(settings.value(QStringLiteral("satLon"), -89.0).toDouble());
    layout->addRow(tr("Longitude (°):"), lonSpin);

    auto* zoomSpin = new QSpinBox(&dlg);
    zoomSpin->setRange(1, 22);
    zoomSpin->setValue(settings.value(QStringLiteral("satZoom"), 15).toInt());
    layout->addRow(tr("Zoom Level:"), zoomSpin);

    auto* opacitySlider = new QSlider(Qt::Horizontal, &dlg);
    opacitySlider->setMinimum(0);
    opacitySlider->setMaximum(100);
    opacitySlider->setValue(80);
    layout->addRow(tr("Opacity:"), opacitySlider);

    auto* fetchBtn = new QPushButton(tr("Fetch Tile"), &dlg);
    fetchBtn->setEnabled(!tokenEdit->text().trimmed().isEmpty());
    layout->addRow(fetchBtn);

    connect(tokenEdit, &QLineEdit::textChanged, this, [fetchBtn](const QString& text) {
        fetchBtn->setEnabled(!text.trimmed().isEmpty());
    });

    connect(opacitySlider, &QSlider::valueChanged, this, [this](int v) {
        m_scene->setSatelliteOpacity(v / 100.0f);
    });

    connect(fetchBtn, &QPushButton::clicked, this, [&]() {
        const QString token   = tokenEdit->text().trimmed();
        const double  lat     = latSpin->value();
        const double  lon     = lonSpin->value();
        const int     zoom    = zoomSpin->value();
        QString url = urlEdit->text();
        url.replace(QStringLiteral("{token}"), token);
        url.replace(QStringLiteral("{lat}"),  QString::number(lat, 'f', 6));
        url.replace(QStringLiteral("{lon}"),  QString::number(lon, 'f', 6));
        url.replace(QStringLiteral("{zoom}"), QString::number(zoom));

        // Save settings
        settings.setValue(QStringLiteral("mapboxToken"),   token);
        settings.setValue(QStringLiteral("mapboxTileUrl"), urlEdit->text());
        settings.setValue(QStringLiteral("satLat"),  lat);
        settings.setValue(QStringLiteral("satLon"),  lon);
        settings.setValue(QStringLiteral("satZoom"), zoom);

        if (m_scene->satelliteItem()) {
            // Detect @2x tile by presence of "@2x" in the URL
            const bool highDpi = url.contains(QStringLiteral("@2x"));
            m_scene->satelliteItem()->setGeoreference(lat, zoom, highDpi);
            m_scene->satelliteItem()->fetchTile(url);
        }
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addRow(buttons);

    dlg.exec();
}

// ---------------------------------------------------------------------------
// Sprint 2-3-9: About dialog
// ---------------------------------------------------------------------------
void MainWindow::showAboutDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("About ARLD"));
    auto* layout = new QVBoxLayout(&dlg);

    auto* titleLabel = new QLabel(
        QStringLiteral("<b>Airshow Ramp Layout Designer</b>"), &dlg);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto* versionLabel = new QLabel(
        tr("Version %1").arg(QLatin1String(ARLD_VERSION_STRING)), &dlg);
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    auto* buildLabel = new QLabel(
        tr("Build date: %1").arg(QLatin1String(__DATE__)), &dlg);
    buildLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(buildLabel);

    auto* copyrightLabel = new QLabel(
        QStringLiteral("© 2026 OpsNormal Airboss"), &dlg);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(copyrightLabel);

    auto* licensesBtn = new QPushButton(tr("View Licenses..."), &dlg);
    layout->addWidget(licensesBtn);

    connect(licensesBtn, &QPushButton::clicked, &dlg, [this, &dlg] {
        // Try to load NOTICES.txt from the source tree or install prefix
        QString noticesText;
        const QStringList searchPaths = {
            QDir::currentPath() + QStringLiteral("/NOTICES.txt"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/NOTICES.txt"),
            QStringLiteral("/usr/local/share/arld/NOTICES.txt"),
        };
        for (const QString& p : searchPaths) {
            QFile f(p);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                noticesText = QString::fromUtf8(f.readAll());
                break;
            }
        }
        if (noticesText.isEmpty())
            noticesText = tr("NOTICES.txt not found. Run 'cmake --build --target generate_notices' to generate it.");

        QDialog licensesDlg(&dlg);
        licensesDlg.setWindowTitle(tr("Third-Party Licenses"));
        licensesDlg.resize(600, 400);
        auto* lLayout = new QVBoxLayout(&licensesDlg);
        auto* browser = new QTextBrowser(&licensesDlg);
        browser->setPlainText(noticesText);
        lLayout->addWidget(browser);
        auto* closeBtn = new QDialogButtonBox(QDialogButtonBox::Close, &licensesDlg);
        connect(closeBtn, &QDialogButtonBox::rejected, &licensesDlg, &QDialog::reject);
        lLayout->addWidget(closeBtn);
        licensesDlg.exec();
    });

    auto* closeBtn = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(closeBtn, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(closeBtn);

    dlg.exec();
}

// ---------------------------------------------------------------------------
// Sprint 2-3-1: Library update dialog
// ---------------------------------------------------------------------------
void MainWindow::showLibraryUpdateDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Check for Library Updates"));
    auto* layout = new QVBoxLayout(&dlg);

    auto* statusLabel = new QLabel(tr("Enter the manifest URL to check for updates."), &dlg);
    statusLabel->setWordWrap(true);
    layout->addWidget(statusLabel);

    auto* urlEdit = new QLineEdit(
        QStringLiteral("https://raw.githubusercontent.com/opsnormal/arld-library/main/library_manifest.json"),
        &dlg);
    layout->addWidget(urlEdit);

    auto* progressBar = new QProgressBar(&dlg);
    progressBar->setVisible(false);
    layout->addWidget(progressBar);

    auto* checker = new arld::ui::LibraryUpdateChecker(&dlg);

    auto* checkBtn  = new QPushButton(tr("Check Now"), &dlg);
    auto* downloadBtn = new QPushButton(tr("Download Updates"), &dlg);
    downloadBtn->setEnabled(false);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(checkBtn);
    btnLayout->addWidget(downloadBtn);
    layout->addLayout(btnLayout);

    QStringList pendingIds;

    connect(checker, &arld::ui::LibraryUpdateChecker::updatesAvailable,
            &dlg, [statusLabel, downloadBtn, &pendingIds](const QStringList& ids) {
        pendingIds = ids;
        statusLabel->setText(tr("%1 update(s) available.").arg(ids.size()));
        downloadBtn->setEnabled(true);
    });
    connect(checker, &arld::ui::LibraryUpdateChecker::upToDate,
            &dlg, [statusLabel] {
        statusLabel->setText(tr("Library is up to date."));
    });
    connect(checker, &arld::ui::LibraryUpdateChecker::checkFailed,
            &dlg, [statusLabel](const QString& error) {
        statusLabel->setText(tr("Check failed: %1").arg(error));
    });
    connect(checker, &arld::ui::LibraryUpdateChecker::downloadProgress,
            &dlg, [progressBar](int current, int total) {
        progressBar->setMaximum(total);
        progressBar->setValue(current);
    });
    connect(checker, &arld::ui::LibraryUpdateChecker::downloadCompleted,
            &dlg, [this, statusLabel, downloadBtn](const QStringList& downloaded) {
        statusLabel->setText(tr("%1 entr%2 updated. Library reloaded.")
            .arg(downloaded.size())
            .arg(downloaded.size() == 1 ? "y" : "ies"));
        downloadBtn->setEnabled(false);
        if (m_libraryPanel)
            m_libraryPanel->reloadLibrary();
    });

    connect(checkBtn, &QPushButton::clicked, &dlg, [checker, urlEdit] {
        checker->checkForUpdates(urlEdit->text().trimmed());
    });

    connect(downloadBtn, &QPushButton::clicked, &dlg,
            [checker, urlEdit, progressBar, &pendingIds] {
        if (!pendingIds.isEmpty()) {
            progressBar->setVisible(true);
            progressBar->setMaximum(pendingIds.size());
            progressBar->setValue(0);
            QString baseUrl = urlEdit->text().trimmed();
            // Strip manifest filename to get base URL
            const int lastSlash = baseUrl.lastIndexOf(QLatin1Char('/'));
            if (lastSlash > 0) baseUrl = baseUrl.left(lastSlash);
            checker->downloadUpdates(pendingIds, baseUrl);
        }
    });

    auto* closeBtns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(closeBtns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(closeBtns);

    dlg.exec();
}

void MainWindow::updateRecentFilesMenu() {
    if (!m_recentFilesMenu) return;
    m_recentFilesMenu->clear();

    QSettings settings(QStringLiteral("OpsNormal Airboss"), QStringLiteral("ARLD"));
    const QStringList recent =
        settings.value(QStringLiteral("recentFiles")).toStringList();

    if (recent.isEmpty()) {
        auto* empty = m_recentFilesMenu->addAction(tr("No recent projects"));
        empty->setEnabled(false);
        return;
    }

    for (const QString& path : recent) {
        const QString name = QFileInfo(path).fileName();
        m_recentFilesMenu->addAction(name, this, [this, path] {
            // Reuse the openProject logic: prompt if dirty, then load.
            if (m_dirty) {
                const auto btn = QMessageBox::question(
                    this, tr("Open Project"),
                    tr("The current layout has unsaved changes. Discard them?"),
                    QMessageBox::Discard | QMessageBox::Cancel,
                    QMessageBox::Cancel);
                if (btn != QMessageBox::Discard) return;
            }
            try {
                auto data = arld::core::ProjectFile::load(path.toStdString());
                auto lookup = [this](const std::string& id)
                    -> const arld::core::AircraftLibraryEntry* {
                    return m_libraryPanel->entryById(id);
                };
                m_scene->loadProjectData(data, lookup);
                m_projectMetadata = data.metadata;
                m_currentFilePath = path;
                m_dirty = false;
                updateWindowTitle();
                updateUndoRedoActions();
                addToRecentFiles(path);
                if (m_violationsPanel)
                    m_violationsPanel->refresh(m_scene->lastViolations());
            } catch (const std::exception& e) {
                QMessageBox::critical(this, tr("Open Failed"),
                                      tr("Could not open project:\n%1").arg(e.what()));
            }
        });
    }
}
