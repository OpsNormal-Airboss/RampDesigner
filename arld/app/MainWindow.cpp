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
#include <arld/export/AircraftManifestExporter.h>
#include <arld/export/BatchExporter.h>
#include <arld/export/PdfExporter.h>
#include <arld/export/SvgExporter.h>
#include <arld/export/ViolationReportExporter.h>
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
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
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringList>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidgetAction>

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

    // Satellite underlay controls (1-5-6)
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Load &Satellite Image..."), this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Load Satellite Image"), QString(),
            tr("Image Files (*.png *.jpg *.jpeg *.tif *.tiff *.bmp);;All Files (*)"));
        if (!path.isEmpty())
            m_scene->setSatelliteImage(path);
    });

    // Opacity slider in a widget action
    auto* opacityLabel = new QLabel(tr("  Satellite Opacity:"));
    viewMenu->addAction(tr("Satellite Opacity"), opacityLabel, nullptr); // placeholder label

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
