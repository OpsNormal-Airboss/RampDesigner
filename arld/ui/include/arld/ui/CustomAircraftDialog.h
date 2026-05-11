#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <QDialog>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

namespace arld::ui {

/// Dialog for creating a custom aircraft entry saved to the user library.
class CustomAircraftDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomAircraftDialog(QWidget* parent = nullptr);

signals:
    /// Emitted (after the dialog is accepted) with the new entry.
    void entryCreated(arld::core::AircraftLibraryEntry entry);

private slots:
    void onBrowseSvg();
    void onAccepted();

private:
    void buildUi();
    bool validateFields();
    std::string generateId() const;

    // Required fields
    QLineEdit*      m_nameEdit        = nullptr;
    QLineEdit*      m_manufacturerEdit = nullptr;
    QLineEdit*      m_modelEdit        = nullptr;
    QLineEdit*      m_variantEdit      = nullptr;
    QComboBox*      m_categoryCombo    = nullptr;

    // Dimensional fields
    QDoubleSpinBox* m_wingspanSpin    = nullptr;
    QDoubleSpinBox* m_lengthSpin      = nullptr;
    QDoubleSpinBox* m_tailHeightSpin  = nullptr;
    QDoubleSpinBox* m_propArcSpin     = nullptr;

    // Display type
    QComboBox*      m_displayTypeCombo = nullptr;

    // SVG silhouette
    QLineEdit*      m_svgPathEdit      = nullptr;
    QPushButton*    m_svgBrowseBtn     = nullptr;
    QLabel*         m_svgPreviewLabel  = nullptr;
    std::string     m_sanitizedSvg;     // sanitized content of the chosen SVG

    // Data sources
    QPlainTextEdit* m_dataSourcesEdit  = nullptr;
};

} // namespace arld::ui
