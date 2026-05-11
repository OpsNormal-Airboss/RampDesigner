#include <arld/ui/ClearanceRuleDialog.h>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace arld::ui {

static QDoubleSpinBox* makeDistanceSpin() {
    auto* sb = new QDoubleSpinBox;
    sb->setMinimum(0.0);
    sb->setMaximum(500.0);
    sb->setSingleStep(1.0);
    sb->setDecimals(1);
    sb->setSuffix(QStringLiteral(" ft"));
    return sb;
}

ClearanceRuleDialog::ClearanceRuleDialog(const arld::core::ClearanceRuleSet& current,
                                          QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Clearance Rules"));
    setMinimumWidth(400);

    auto* layout = new QVBoxLayout(this);

    // Template picker
    m_templateCombo = new QComboBox(this);
    m_templateCombo->addItem(tr("FAA CoW (Default)"));
    m_templateCombo->addItem(tr("ICAS Standard"));
    layout->addWidget(new QLabel(tr("Template:"), this));
    layout->addWidget(m_templateCombo);

    // Spinboxes via form layout
    auto* form = new QFormLayout;
    m_staticDisplay   = makeDistanceSpin();
    m_warbirdBonus    = makeDistanceSpin();
    m_taxiOnly        = makeDistanceSpin();
    m_militaryStatic  = makeDistanceSpin();
    m_hotRamp         = makeDistanceSpin();
    m_mediaPlatform   = makeDistanceSpin();
    m_rampShow        = makeDistanceSpin();

    form->addRow(tr("Static Display wingtip clearance:"), m_staticDisplay);
    form->addRow(tr("Warbird/Heritage prop arc bonus:"),  m_warbirdBonus);
    form->addRow(tr("Taxi-only corridor:"),               m_taxiOnly);
    form->addRow(tr("Military static standoff:"),         m_militaryStatic);
    form->addRow(tr("Hot ramp standoff:"),                m_hotRamp);
    form->addRow(tr("Media/photo platform barrier:"),     m_mediaPlatform);
    form->addRow(tr("Ramp show crowd line:"),             m_rampShow);
    layout->addLayout(form);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_templateCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ClearanceRuleDialog::loadTemplate);

    // Populate from current ruleset
    m_staticDisplay->setValue(static_cast<double>(current.staticDisplayWingtipFt));
    m_warbirdBonus->setValue(static_cast<double>(current.warbirdPropArcBonusFt));
    m_taxiOnly->setValue(static_cast<double>(current.taxiOnlyCorridorFt));
    m_militaryStatic->setValue(static_cast<double>(current.militaryStaticStandoffFt));
    m_hotRamp->setValue(static_cast<double>(current.hotRampStandoffFt));
    m_mediaPlatform->setValue(static_cast<double>(current.mediaPhotoPlatformFt));
    m_rampShow->setValue(static_cast<double>(current.rampShowCrowdLineFt));
}

arld::core::ClearanceRuleSet ClearanceRuleDialog::ruleSet() const {
    arld::core::ClearanceRuleSet rs;
    rs.staticDisplayWingtipFt   = static_cast<float>(m_staticDisplay->value());
    rs.warbirdPropArcBonusFt    = static_cast<float>(m_warbirdBonus->value());
    rs.taxiOnlyCorridorFt       = static_cast<float>(m_taxiOnly->value());
    rs.militaryStaticStandoffFt = static_cast<float>(m_militaryStatic->value());
    rs.hotRampStandoffFt        = static_cast<float>(m_hotRamp->value());
    rs.mediaPhotoPlatformFt     = static_cast<float>(m_mediaPlatform->value());
    rs.rampShowCrowdLineFt      = static_cast<float>(m_rampShow->value());
    return rs;
}

void ClearanceRuleDialog::loadTemplate(int index) {
    arld::core::ClearanceRuleSet rs =
        (index == 1) ? arld::core::ClearanceRuleSet::icas()
                     : arld::core::ClearanceRuleSet::faaCoW();

    m_staticDisplay->setValue(static_cast<double>(rs.staticDisplayWingtipFt));
    m_warbirdBonus->setValue(static_cast<double>(rs.warbirdPropArcBonusFt));
    m_taxiOnly->setValue(static_cast<double>(rs.taxiOnlyCorridorFt));
    m_militaryStatic->setValue(static_cast<double>(rs.militaryStaticStandoffFt));
    m_hotRamp->setValue(static_cast<double>(rs.hotRampStandoffFt));
    m_mediaPlatform->setValue(static_cast<double>(rs.mediaPhotoPlatformFt));
    m_rampShow->setValue(static_cast<double>(rs.rampShowCrowdLineFt));
}

} // namespace arld::ui
