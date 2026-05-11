#include <arld/ui/ClearanceOverrideDialog.h>
#include <arld/core/ProjectFile.h>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace arld::ui {

static constexpr int kMinJustificationChars = 20;

ClearanceOverrideDialog::ClearanceOverrideDialog(const QString& idA, const QString& idB,
                                                  float measuredFt, float requiredFt,
                                                  QWidget* parent)
    : QDialog(parent)
    , m_idA(idA)
    , m_idB(idB) {
    setWindowTitle(tr("Override Clearance Violation"));
    setMinimumWidth(500);

    auto* layout = new QVBoxLayout(this);

    // Info labels
    auto* form = new QFormLayout;
    form->addRow(tr("Aircraft A:"),       new QLabel(idA, this));
    form->addRow(tr("Aircraft B:"),       new QLabel(idB, this));
    form->addRow(tr("Measured gap:"),
        new QLabel(tr("%1 ft").arg(static_cast<double>(measuredFt), 0, 'f', 1), this));
    form->addRow(tr("Required gap:"),
        new QLabel(tr("%1 ft").arg(static_cast<double>(requiredFt), 0, 'f', 1), this));
    layout->addLayout(form);

    // Justification text edit
    auto* justLabel = new QLabel(tr("Justification (minimum 20 characters):"), this);
    layout->addWidget(justLabel);

    m_justificationEdit = new QTextEdit(this);
    m_justificationEdit->setPlaceholderText(
        tr("Describe the operational reason for permitting this clearance exception..."));
    m_justificationEdit->setMinimumHeight(100);
    layout->addWidget(m_justificationEdit);

    // Character counter
    m_charCountLabel = new QLabel(tr("0 / %1 minimum").arg(kMinJustificationChars), this);
    m_charCountLabel->setStyleSheet(QStringLiteral("color: #CC2222;"));
    layout->addWidget(m_charCountLabel);

    // Buttons
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    layout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_justificationEdit, &QTextEdit::textChanged,
            this, &ClearanceOverrideDialog::onTextChanged);
}

arld::core::ClearanceOverride ClearanceOverrideDialog::result() const {
    arld::core::ClearanceOverride co;
    co.placementIdA  = m_idA.toStdString();
    co.placementIdB  = m_idB.toStdString();
    co.justification = m_justificationEdit->toPlainText().toStdString();
    // Try $USER env var; fall back to empty string
    const QByteArray user = qgetenv("USER");
    co.username      = user.isEmpty() ? std::string("") : std::string(user.constData());
    co.timestampUtc  = arld::core::ProjectFile::currentUtcTimestamp();
    return co;
}

void ClearanceOverrideDialog::onTextChanged() {
    const int len = m_justificationEdit->toPlainText().length();
    const bool sufficient = (len >= kMinJustificationChars);

    m_charCountLabel->setText(
        tr("%1 / %2 minimum").arg(len).arg(kMinJustificationChars));
    m_charCountLabel->setStyleSheet(
        sufficient ? QStringLiteral("color: #00AA33;")
                   : QStringLiteral("color: #CC2222;"));

    if (m_buttonBox->button(QDialogButtonBox::Ok))
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(sufficient);
}

} // namespace arld::ui
