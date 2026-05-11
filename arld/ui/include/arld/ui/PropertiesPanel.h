#pragma once
#include <QDockWidget>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QLabel;

namespace arld::ui {

class AircraftItem;

class PropertiesPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    /// Set the currently selected aircraft (nullptr = clear / show placeholder).
    void setAircraft(AircraftItem* item);

private slots:
    void onDisplayTypeChanged(int index);
    void onTailNumberChanged(const QString& text);
    void onOwnerChanged(const QString& text);
    void onFuelTypeChanged(const QString& text);
    void onHazmatToggled(bool checked);

private:
    AircraftItem* m_current = nullptr;
    QLabel*    m_nameLabel;
    QComboBox* m_displayTypeCombo;
    QLineEdit* m_tailNumberEdit;
    QLineEdit* m_ownerEdit;
    QLineEdit* m_fuelTypeEdit;
    QCheckBox* m_hazmatCheck;
    QWidget*   m_content;
    QWidget*   m_placeholder;
};

} // namespace arld::ui
