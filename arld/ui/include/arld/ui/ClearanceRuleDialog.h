#pragma once
#include <arld/core/ClearanceRuleSet.h>
#include <QDialog>

class QComboBox;
class QDoubleSpinBox;
class QDialogButtonBox;

namespace arld::ui {

class ClearanceRuleDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClearanceRuleDialog(const arld::core::ClearanceRuleSet& current,
                                  QWidget* parent = nullptr);
    arld::core::ClearanceRuleSet ruleSet() const;

private slots:
    void loadTemplate(int index);

private:
    QComboBox*        m_templateCombo;
    QDoubleSpinBox*   m_staticDisplay;
    QDoubleSpinBox*   m_warbirdBonus;
    QDoubleSpinBox*   m_taxiOnly;
    QDoubleSpinBox*   m_militaryStatic;
    QDoubleSpinBox*   m_hotRamp;
    QDoubleSpinBox*   m_mediaPlatform;
    QDoubleSpinBox*   m_rampShow;
    QDialogButtonBox* m_buttonBox;
};

} // namespace arld::ui
