#include <arld/ui/PropertiesPanel.h>
#include <arld/ui/AircraftItem.h>
#include <arld/core/AircraftLibraryEntry.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>

namespace arld::ui {

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QDockWidget(tr("Properties"), parent) {
    setObjectName(QStringLiteral("PropertiesPanel"));
    setMinimumWidth(220);

    // Build content widget (shown when an aircraft is selected)
    m_content = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(8, 8, 8, 8);

    m_nameLabel = new QLabel(m_content);
    m_nameLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    contentLayout->addWidget(m_nameLabel);

    auto* form = new QFormLayout;
    contentLayout->addLayout(form);

    m_displayTypeCombo = new QComboBox(m_content);
    m_displayTypeCombo->addItem(tr("Static Display"));
    m_displayTypeCombo->addItem(tr("Warbird / Heritage"));
    m_displayTypeCombo->addItem(tr("Taxi Only"));
    m_displayTypeCombo->addItem(tr("Military Static"));
    m_displayTypeCombo->addItem(tr("Hot Ramp"));
    m_displayTypeCombo->addItem(tr("Media / Photo Platform"));
    m_displayTypeCombo->addItem(tr("Ramp Show"));
    form->addRow(tr("Display Type:"), m_displayTypeCombo);

    // Gear state (1-4-2): only visible for aircraft with retractable gear.
    m_gearCombo = new QComboBox(m_content);
    m_gearCombo->addItem(tr("Gear Extended (Down)"));
    m_gearCombo->addItem(tr("Gear Retracted (Up)"));
    m_gearLabel = new QLabel(tr("Gear State:"), m_content);
    form->addRow(m_gearLabel, m_gearCombo);
    m_gearCombo->setVisible(false);
    m_gearLabel->setVisible(false);

    m_tailNumberEdit = new QLineEdit(m_content);
    m_tailNumberEdit->setPlaceholderText(tr("e.g. NX71JB"));
    form->addRow(tr("Tail Number:"), m_tailNumberEdit);

    m_ownerEdit = new QLineEdit(m_content);
    m_ownerEdit->setPlaceholderText(tr("e.g. EAA AirVenture"));
    form->addRow(tr("Owner:"), m_ownerEdit);

    m_fuelTypeEdit = new QLineEdit(m_content);
    m_fuelTypeEdit->setPlaceholderText(tr("e.g. 100LL"));
    form->addRow(tr("Fuel Type:"), m_fuelTypeEdit);

    m_hazmatCheck = new QCheckBox(tr("Carries hazardous materials"), m_content);
    contentLayout->addWidget(m_hazmatCheck);

    // --- Heading controls (1-3-5) ---
    m_headingEdit = new QSpinBox(m_content);
    m_headingEdit->setRange(0, 359);
    m_headingEdit->setWrapping(true);
    m_headingEdit->setSuffix(tr("°"));
    form->addRow(tr("Heading (°):"), m_headingEdit);

    m_snapHeadingBtn = new QPushButton(tr("Snap All Selected to This Heading"), m_content);
    contentLayout->addWidget(m_snapHeadingBtn);

    contentLayout->addStretch();

    // Build placeholder widget (shown when nothing is selected)
    m_placeholder = new QWidget(this);
    auto* phLayout = new QVBoxLayout(m_placeholder);
    phLayout->setContentsMargins(8, 8, 8, 8);
    auto* phLabel = new QLabel(tr("Select an aircraft to view properties"), m_placeholder);
    phLabel->setWordWrap(true);
    phLabel->setAlignment(Qt::AlignCenter);
    phLabel->setStyleSheet(QStringLiteral("color: #888888;"));
    phLayout->addWidget(phLabel);

    // Use a stacked widget to swap between placeholder and content
    auto* stack = new QStackedWidget(this);
    stack->addWidget(m_placeholder);  // index 0
    stack->addWidget(m_content);      // index 1
    setWidget(stack);

    // Default: show placeholder
    stack->setCurrentIndex(0);

    // Keyboard navigation (1-4-8): focus proxy on display type combo.
    setFocusProxy(m_displayTypeCombo);

    // Wire signals
    connect(m_displayTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PropertiesPanel::onDisplayTypeChanged);
    connect(m_tailNumberEdit, &QLineEdit::textEdited,
            this, &PropertiesPanel::onTailNumberChanged);
    connect(m_ownerEdit, &QLineEdit::textEdited,
            this, &PropertiesPanel::onOwnerChanged);
    connect(m_fuelTypeEdit, &QLineEdit::textEdited,
            this, &PropertiesPanel::onFuelTypeChanged);
    connect(m_hazmatCheck, &QCheckBox::toggled,
            this, &PropertiesPanel::onHazmatToggled);
    connect(m_headingEdit, qOverload<int>(&QSpinBox::valueChanged),
            this, &PropertiesPanel::onHeadingChanged);
    connect(m_snapHeadingBtn, &QPushButton::clicked,
            this, &PropertiesPanel::onSnapHeading);
    connect(m_gearCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PropertiesPanel::onGearStateChanged);
}

void PropertiesPanel::setAircraft(AircraftItem* item) {
    m_current = item;

    auto* stack = qobject_cast<QStackedWidget*>(widget());
    if (!stack) return;

    if (!item) {
        stack->setCurrentIndex(0);  // show placeholder
        return;
    }

    // Populate fields without triggering feedback loops
    m_displayTypeCombo->blockSignals(true);
    m_tailNumberEdit->blockSignals(true);
    m_ownerEdit->blockSignals(true);
    m_fuelTypeEdit->blockSignals(true);
    m_hazmatCheck->blockSignals(true);
    m_headingEdit->blockSignals(true);
    m_gearCombo->blockSignals(true);

    m_nameLabel->setText(QString::fromStdString(item->entry().displayName));
    m_displayTypeCombo->setCurrentIndex(
        static_cast<int>(item->displayType()));
    m_tailNumberEdit->setText(QString::fromStdString(item->tailNumber()));
    m_ownerEdit->setText(QString::fromStdString(item->owner()));
    m_fuelTypeEdit->setText(QString::fromStdString(item->fuelType()));
    m_hazmatCheck->setChecked(item->hasHazmat());
    m_headingEdit->setValue(static_cast<int>(std::round(item->rotation())));

    // Gear state: only show for aircraft with retractable gear.
    const bool hasGear = item->entry().hasRetractableGear;
    m_gearCombo->setVisible(hasGear);
    m_gearLabel->setVisible(hasGear);
    if (hasGear) {
        m_gearCombo->setCurrentIndex(item->gearExtended() ? 0 : 1);
    }

    m_displayTypeCombo->blockSignals(false);
    m_tailNumberEdit->blockSignals(false);
    m_ownerEdit->blockSignals(false);
    m_fuelTypeEdit->blockSignals(false);
    m_hazmatCheck->blockSignals(false);
    m_headingEdit->blockSignals(false);
    m_gearCombo->blockSignals(false);

    stack->setCurrentIndex(1);  // show content
}

void PropertiesPanel::onDisplayTypeChanged(int index) {
    if (!m_current) return;
    using DT = arld::core::DisplayType;
    m_current->setDisplayType(static_cast<DT>(index));
}

void PropertiesPanel::onTailNumberChanged(const QString& text) {
    if (!m_current) return;
    m_current->setTailNumber(text.toStdString());
}

void PropertiesPanel::onOwnerChanged(const QString& text) {
    if (!m_current) return;
    m_current->setOwner(text.toStdString());
}

void PropertiesPanel::onFuelTypeChanged(const QString& text) {
    if (!m_current) return;
    m_current->setFuelType(text.toStdString());
}

void PropertiesPanel::onHazmatToggled(bool checked) {
    if (!m_current) return;
    m_current->setHazmat(checked);
}

void PropertiesPanel::onHeadingChanged(int degrees) {
    if (!m_current) return;
    const double fromDeg = m_current->rotation();
    const double toDeg   = static_cast<double>(degrees);
    if (std::abs(toDeg - fromDeg) < 0.01) return;
    m_current->setRotation(toDeg);
    m_current->commitRotation(fromDeg, toDeg);
}

void PropertiesPanel::onGearStateChanged(int idx) {
    if (!m_current) return;
    m_current->setGearExtended(idx == 0);
}

void PropertiesPanel::onSnapHeading() {
    if (!m_current) return;
    if (!m_current->scene()) return;

    const int heading = m_headingEdit->value();

    // Apply the heading to every selected AircraftItem in the scene.
    const auto selected = m_current->scene()->selectedItems();
    for (auto* sceneItem : selected) {
        auto* ac = qgraphicsitem_cast<AircraftItem*>(sceneItem);
        if (!ac) continue;
        const double fromDeg = ac->rotation();
        const double toDeg   = static_cast<double>(heading);
        if (std::abs(toDeg - fromDeg) < 0.01) continue;
        ac->setRotation(toDeg);
        ac->commitRotation(fromDeg, toDeg);
    }
}

} // namespace arld::ui
