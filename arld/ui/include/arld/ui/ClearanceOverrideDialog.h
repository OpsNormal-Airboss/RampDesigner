#pragma once
#include <arld/core/ProjectFile.h>
#include <QDialog>

class QTextEdit;
class QLabel;
class QDialogButtonBox;

namespace arld::ui {

class ClearanceOverrideDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClearanceOverrideDialog(const QString& idA, const QString& idB,
                                      float measuredFt, float requiredFt,
                                      QWidget* parent = nullptr);
    arld::core::ClearanceOverride result() const;

private slots:
    void onTextChanged();

private:
    QString           m_idA, m_idB;
    QTextEdit*        m_justificationEdit;
    QLabel*           m_charCountLabel;
    QDialogButtonBox* m_buttonBox;
};

} // namespace arld::ui
