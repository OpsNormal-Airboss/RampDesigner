#include <arld/ui/VersionsPanel.h>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace arld::ui {

VersionsPanel::VersionsPanel(QWidget* parent)
    : QDockWidget(tr("Layout Versions"), parent)
{
    setObjectName(QStringLiteral("VersionsPanel"));

    auto* container = new QWidget(this);
    auto* vlay = new QVBoxLayout(container);
    vlay->setContentsMargins(4, 4, 4, 4);

    m_list = new QListWidget(container);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    vlay->addWidget(m_list);

    // Button row
    auto* btnRow = new QWidget(container);
    auto* hlay   = new QHBoxLayout(btnRow);
    hlay->setContentsMargins(0, 0, 0, 0);
    hlay->setSpacing(4);

    m_saveBtn   = new QPushButton(tr("Save Current…"), btnRow);
    m_switchBtn = new QPushButton(tr("Switch To"),     btnRow);
    m_exportBtn = new QPushButton(tr("Export…"),       btnRow);
    m_deltaBtn  = new QPushButton(tr("Compare Δ…"),   btnRow);

    m_switchBtn->setEnabled(false);
    m_exportBtn->setEnabled(false);
    m_deltaBtn->setEnabled(false);

    hlay->addWidget(m_saveBtn);
    hlay->addWidget(m_switchBtn);
    hlay->addWidget(m_exportBtn);
    hlay->addWidget(m_deltaBtn);

    vlay->addWidget(btnRow);
    setWidget(container);

    connect(m_saveBtn,   &QPushButton::clicked, this, &VersionsPanel::onSaveClicked);
    connect(m_switchBtn, &QPushButton::clicked, this, &VersionsPanel::onSwitchClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &VersionsPanel::onExportClicked);
    connect(m_deltaBtn,  &QPushButton::clicked, this, &VersionsPanel::onDeltaClicked);
    connect(m_list, &QListWidget::itemSelectionChanged,
            this, &VersionsPanel::onSelectionChanged);
}

void VersionsPanel::setProjectData(const arld::core::ProjectData& data) {
    m_versions = data.versions;
    m_list->clear();
    for (const auto& ver : m_versions) {
        const QString label = QString("%1  [%2]")
            .arg(QString::fromStdString(ver.name))
            .arg(QString::fromStdString(ver.createdUtc).left(10));
        m_list->addItem(label);
    }
    onSelectionChanged();
}

void VersionsPanel::onSelectionChanged() {
    const int sel = m_list->selectedItems().size();
    m_switchBtn->setEnabled(sel == 1);
    m_exportBtn->setEnabled(sel == 1);
    m_deltaBtn->setEnabled(sel == 2);
}

void VersionsPanel::onSaveClicked() {
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("Save Layout Version"),
        tr("Version name:"), QLineEdit::Normal,
        tr("Version %1").arg(static_cast<int>(m_versions.size()) + 1),
        &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    emit versionSaveRequested(name.trimmed());
}

void VersionsPanel::onSwitchClicked() {
    const auto sel = m_list->selectedItems();
    if (sel.isEmpty()) return;
    const int row = m_list->row(sel.first());
    if (row < 0 || row >= static_cast<int>(m_versions.size())) return;
    emit versionSwitchRequested(QString::fromStdString(m_versions[row].id));
}

void VersionsPanel::onExportClicked() {
    const auto sel = m_list->selectedItems();
    if (sel.isEmpty()) return;
    const int row = m_list->row(sel.first());
    if (row < 0 || row >= static_cast<int>(m_versions.size())) return;
    emit versionExportRequested(QString::fromStdString(m_versions[row].id));
}

void VersionsPanel::onDeltaClicked() {
    const auto sel = m_list->selectedItems();
    if (sel.size() != 2) return;
    // rows in order
    int rowA = m_list->row(sel[0]);
    int rowB = m_list->row(sel[1]);
    if (rowA > rowB) std::swap(rowA, rowB);
    if (rowA < 0 || rowB >= static_cast<int>(m_versions.size())) return;
    emit versionDeltaRequested(
        QString::fromStdString(m_versions[rowA].id),
        QString::fromStdString(m_versions[rowB].id));
}

} // namespace arld::ui
