#pragma once
#include <QDockWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

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
    void onHeadingChanged(int degrees);
    void onSnapHeading();
    void onGearStateChanged(int index);
    void onArrivalTimeChanged(const QString& text);
    void onDepartureTimeChanged(const QString& text);

private:
    AircraftItem* m_current = nullptr;
    QLabel*    m_nameLabel;
    QComboBox* m_displayTypeCombo;
    QComboBox* m_gearCombo        = nullptr;  // gear state: extended / retracted
    QLabel*    m_gearLabel        = nullptr;  // label row for gear combo
    QLineEdit* m_tailNumberEdit;
    QLineEdit* m_ownerEdit;
    QLineEdit* m_fuelTypeEdit;
    QCheckBox* m_hazmatCheck;
    QSpinBox*  m_headingEdit    = nullptr;
    QPushButton* m_snapHeadingBtn = nullptr;
    QLineEdit* m_arrivalEdit    = nullptr;
    QLineEdit* m_departureEdit  = nullptr;
    QWidget*   m_content;
    QWidget*   m_placeholder;
};

} // namespace arld::ui
