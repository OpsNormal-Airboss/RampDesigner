#include <arld/ui/ViolationsPanel.h>
#include <arld/ui/ClearanceOverrideDialog.h>
#include <arld/ui/RampScene.h>
#include <arld/ui/RampView.h>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace arld::ui {

static constexpr int kColAirA      = 0;
static constexpr int kColAirB      = 1;
static constexpr int kColSeverity  = 2;
static constexpr int kColMeasured  = 3;
static constexpr int kColRequired  = 4;
static constexpr int kNumCols      = 5;

ViolationsPanel::ViolationsPanel(RampScene* scene, RampView* view, QWidget* parent)
    : QDockWidget(tr("Clearance Violations"), parent)
    , m_scene(scene)
    , m_view(view) {
    setObjectName(QStringLiteral("ViolationsPanel"));

    auto* container = new QWidget(this);
    auto* layout    = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);

    m_table = new QTableWidget(0, kNumCols, container);
    m_table->setHorizontalHeaderLabels({
        tr("Aircraft A"), tr("Aircraft B"),
        tr("Severity"), tr("Measured (ft)"), tr("Required (ft)")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table);

    auto* btnRow = new QHBoxLayout;
    m_overrideBtn = new QPushButton(tr("Override..."), container);
    m_overrideBtn->setEnabled(false);
    btnRow->addStretch();
    btnRow->addWidget(m_overrideBtn);
    layout->addLayout(btnRow);

    setWidget(container);

    connect(m_table, &QTableWidget::cellClicked,
            this, &ViolationsPanel::onRowClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection&, const QItemSelection&) {
        const auto sel = m_table->selectionModel()->selectedRows();
        bool canOverride = false;
        if (!sel.isEmpty()) {
            int row = sel.first().row();
            if (row < static_cast<int>(m_violations.size())) {
                const auto& v = m_violations[row];
                canOverride = (v.severity == arld::core::ClearanceSeverity::Violation);
            }
        }
        m_overrideBtn->setEnabled(canOverride);
    });
    connect(m_overrideBtn, &QPushButton::clicked,
            this, &ViolationsPanel::onOverrideClicked);

    refresh({});
}

void ViolationsPanel::refresh(const std::vector<arld::core::ViolationResult>& violations) {
    m_violations = violations;
    m_table->setRowCount(0);

    if (violations.empty()) {
        m_table->setRowCount(1);
        auto* item = new QTableWidgetItem(tr("✓ No violations"));
        item->setForeground(QColor(0x00, 0xAA, 0x33));
        m_table->setItem(0, 0, item);
        // Span all columns
        m_table->setSpan(0, 0, 1, kNumCols);
        m_overrideBtn->setEnabled(false);
        return;
    }

    // Remove any previous span
    m_table->setSpan(0, 0, 1, 1);

    m_table->setRowCount(static_cast<int>(violations.size()));
    for (int row = 0; row < static_cast<int>(violations.size()); ++row) {
        const auto& v = violations[row];

        auto* itemA = new QTableWidgetItem(QString::fromStdString(v.idA));
        auto* itemB = new QTableWidgetItem(QString::fromStdString(v.idB));

        QTableWidgetItem* itemSev;
        if (v.severity == arld::core::ClearanceSeverity::Violation) {
            itemSev = new QTableWidgetItem(tr("✖ Violation"));
            itemSev->setForeground(QColor(0xCC, 0x22, 0x22));
        } else if (v.severity == arld::core::ClearanceSeverity::Advisory) {
            itemSev = new QTableWidgetItem(tr("⚠ Advisory"));
            itemSev->setForeground(QColor(0xDD, 0xAA, 0x00));
        } else {
            // Overridden
            itemSev = new QTableWidgetItem(tr("○ Overridden"));
            itemSev->setForeground(QColor(0xDD, 0x77, 0x00));
        }

        auto* itemMeas = new QTableWidgetItem(
            QString::number(static_cast<double>(v.separationFt), 'f', 1));
        auto* itemReq  = new QTableWidgetItem(
            QString::number(static_cast<double>(v.requiredFt), 'f', 1));

        m_table->setItem(row, kColAirA,     itemA);
        m_table->setItem(row, kColAirB,     itemB);
        m_table->setItem(row, kColSeverity, itemSev);
        m_table->setItem(row, kColMeasured, itemMeas);
        m_table->setItem(row, kColRequired, itemReq);
    }
}

void ViolationsPanel::onRowClicked(int row, int /*col*/) {
    if (row < 0 || row >= static_cast<int>(m_violations.size())) return;
    const auto& v = m_violations[row];

    const QPointF centerA = m_scene->aircraftSceneCenter(v.idA);
    const QPointF centerB = m_scene->aircraftSceneCenter(v.idB);
    const QPointF midPoint((centerA.x() + centerB.x()) / 2.0,
                           (centerA.y() + centerB.y()) / 2.0);
    m_view->centerOn(midPoint);
}

void ViolationsPanel::onOverrideClicked() {
    const auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    int row = sel.first().row();
    if (row >= static_cast<int>(m_violations.size())) return;

    const auto& v = m_violations[row];
    if (v.severity != arld::core::ClearanceSeverity::Violation) return;

    ClearanceOverrideDialog dlg(
        QString::fromStdString(v.idA),
        QString::fromStdString(v.idB),
        v.separationFt,
        v.requiredFt,
        this);

    if (dlg.exec() == QDialog::Accepted) {
        m_scene->addOverride(dlg.result());
    }
}

} // namespace arld::ui
