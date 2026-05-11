#include <arld/ui/CustomAircraftDialog.h>
#include <arld/core/SvgSanitizer.h>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIODevice>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <ctime>

namespace arld::ui {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string slugify(const std::string& s) {
    std::string result;
    result.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c)) {
            result += static_cast<char>(std::tolower(c));
        } else if (c == ' ' || c == '_' || c == '-' || c == '/') {
            if (!result.empty() && result.back() != '-')
                result += '-';
        }
    }
    // Trim trailing hyphen
    while (!result.empty() && result.back() == '-')
        result.pop_back();
    return result;
}

static QString todayIso() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::gmtime(&t);
    char buf[12];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return QString::fromLatin1(buf);
}

// ---------------------------------------------------------------------------
// CustomAircraftDialog
// ---------------------------------------------------------------------------

CustomAircraftDialog::CustomAircraftDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Add Custom Aircraft"));
    setMinimumWidth(520);
    buildUi();

    connect(this, &QDialog::accepted, this, &CustomAircraftDialog::onAccepted);
}

void CustomAircraftDialog::buildUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // Use a scroll area so the dialog stays usable on small screens.
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget;
    scroll->setWidget(content);
    mainLayout->addWidget(scroll);

    auto* form = new QFormLayout(content);
    form->setContentsMargins(8, 8, 8, 8);
    form->setSpacing(6);

    // --- Identity ---
    m_nameEdit = new QLineEdit(content);
    m_nameEdit->setPlaceholderText(tr("e.g. P-51D Mustang"));
    form->addRow(tr("Display Name*:"), m_nameEdit);

    m_manufacturerEdit = new QLineEdit(content);
    m_manufacturerEdit->setPlaceholderText(tr("e.g. North American Aviation"));
    form->addRow(tr("Manufacturer*:"), m_manufacturerEdit);

    m_modelEdit = new QLineEdit(content);
    m_modelEdit->setPlaceholderText(tr("e.g. P-51"));
    form->addRow(tr("Model*:"), m_modelEdit);

    m_variantEdit = new QLineEdit(content);
    m_variantEdit->setPlaceholderText(tr("e.g. D (optional)"));
    form->addRow(tr("Variant:"), m_variantEdit);

    // --- Category ---
    m_categoryCombo = new QComboBox(content);
    m_categoryCombo->addItem(tr("Warbird"),         static_cast<int>(arld::core::AircraftCategory::Warbird));
    m_categoryCombo->addItem(tr("Jet Fighter"),     static_cast<int>(arld::core::AircraftCategory::JetFighter));
    m_categoryCombo->addItem(tr("Heavy Transport"), static_cast<int>(arld::core::AircraftCategory::HeavyTransport));
    m_categoryCombo->addItem(tr("Bomber"),          static_cast<int>(arld::core::AircraftCategory::Bomber));
    m_categoryCombo->addItem(tr("General Aviation"),static_cast<int>(arld::core::AircraftCategory::GeneralAviation));
    m_categoryCombo->addItem(tr("Aerobatic"),       static_cast<int>(arld::core::AircraftCategory::Aerobatic));
    m_categoryCombo->addItem(tr("Helicopter"),      static_cast<int>(arld::core::AircraftCategory::Helicopter));
    m_categoryCombo->addItem(tr("Business Jet"),    static_cast<int>(arld::core::AircraftCategory::BusinessJet));
    form->addRow(tr("Category*:"), m_categoryCombo);

    // --- Dimensions ---
    m_wingspanSpin = new QDoubleSpinBox(content);
    m_wingspanSpin->setRange(5.0, 500.0);
    m_wingspanSpin->setSuffix(tr(" ft"));
    m_wingspanSpin->setDecimals(1);
    m_wingspanSpin->setValue(40.0);
    form->addRow(tr("Wingspan*:"), m_wingspanSpin);

    m_lengthSpin = new QDoubleSpinBox(content);
    m_lengthSpin->setRange(5.0, 500.0);
    m_lengthSpin->setSuffix(tr(" ft"));
    m_lengthSpin->setDecimals(1);
    m_lengthSpin->setValue(30.0);
    form->addRow(tr("Length*:"), m_lengthSpin);

    m_tailHeightSpin = new QDoubleSpinBox(content);
    m_tailHeightSpin->setRange(1.0, 100.0);
    m_tailHeightSpin->setSuffix(tr(" ft"));
    m_tailHeightSpin->setDecimals(1);
    m_tailHeightSpin->setValue(12.0);
    form->addRow(tr("Tail Height*:"), m_tailHeightSpin);

    m_propArcSpin = new QDoubleSpinBox(content);
    m_propArcSpin->setRange(0.0, 50.0);
    m_propArcSpin->setSuffix(tr(" ft"));
    m_propArcSpin->setDecimals(1);
    m_propArcSpin->setValue(0.0);
    m_propArcSpin->setSpecialValueText(tr("None"));
    form->addRow(tr("Prop Arc (0 = none):"), m_propArcSpin);

    // --- Display type ---
    m_displayTypeCombo = new QComboBox(content);
    m_displayTypeCombo->addItem(tr("Static Display"),        static_cast<int>(arld::core::DisplayType::StaticDisplay));
    m_displayTypeCombo->addItem(tr("Warbird / Heritage"),    static_cast<int>(arld::core::DisplayType::WarbirdHeritage));
    m_displayTypeCombo->addItem(tr("Taxi Only"),             static_cast<int>(arld::core::DisplayType::TaxiOnly));
    m_displayTypeCombo->addItem(tr("Military Static"),       static_cast<int>(arld::core::DisplayType::MilitaryStatic));
    m_displayTypeCombo->addItem(tr("Hot Ramp"),              static_cast<int>(arld::core::DisplayType::HotRamp));
    m_displayTypeCombo->addItem(tr("Media / Photo Platform"),static_cast<int>(arld::core::DisplayType::MediaPhotoPlatform));
    m_displayTypeCombo->addItem(tr("Ramp Show"),             static_cast<int>(arld::core::DisplayType::RampShow));
    form->addRow(tr("Default Display Type*:"), m_displayTypeCombo);

    // --- SVG silhouette ---
    auto* svgRow = new QHBoxLayout;
    m_svgPathEdit = new QLineEdit(content);
    m_svgPathEdit->setReadOnly(true);
    m_svgPathEdit->setPlaceholderText(tr("Optional — select an SVG silhouette"));
    m_svgBrowseBtn = new QPushButton(tr("Browse..."), content);
    svgRow->addWidget(m_svgPathEdit);
    svgRow->addWidget(m_svgBrowseBtn);
    form->addRow(tr("SVG Silhouette:"), svgRow);

    m_svgPreviewLabel = new QLabel(content);
    m_svgPreviewLabel->setFixedSize(64, 64);
    m_svgPreviewLabel->setAlignment(Qt::AlignCenter);
    m_svgPreviewLabel->setStyleSheet("border: 1px solid #999; background: #F8F8F8;");
    m_svgPreviewLabel->setText(tr("No SVG"));
    form->addRow(QString(), m_svgPreviewLabel);

    connect(m_svgBrowseBtn, &QPushButton::clicked,
            this, &CustomAircraftDialog::onBrowseSvg);

    // --- Data sources ---
    m_dataSourcesEdit = new QPlainTextEdit(content);
    m_dataSourcesEdit->setPlaceholderText(
        tr("One source per line (at least 1 required)\ne.g. manufacturer specifications"));
    m_dataSourcesEdit->setMaximumHeight(80);
    form->addRow(tr("Data Sources*:"), m_dataSourcesEdit);

    // --- Buttons ---
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CustomAircraftDialog::onBrowseSvg() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Select SVG Silhouette"), QString(),
        tr("SVG Files (*.svg);;All Files (*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("SVG Load Error"),
                             tr("Could not read the selected file."));
        return;
    }
    const std::string raw = f.readAll().toStdString();
    f.close();

    m_sanitizedSvg = arld::core::SvgSanitizer::sanitize(raw);
    m_svgPathEdit->setText(path);

    // Render preview
    QSvgRenderer renderer(QByteArray::fromStdString(m_sanitizedSvg));
    if (renderer.isValid()) {
        QPixmap px(64, 64);
        px.fill(Qt::transparent);
        QPainter p(&px);
        renderer.render(&p);
        m_svgPreviewLabel->setPixmap(px);
        m_svgPreviewLabel->setText(QString());
    } else {
        m_svgPreviewLabel->setText(tr("Invalid SVG"));
    }
}

bool CustomAircraftDialog::validateFields() {
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Display Name is required."));
        m_nameEdit->setFocus();
        return false;
    }
    if (m_manufacturerEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Manufacturer is required."));
        m_manufacturerEdit->setFocus();
        return false;
    }
    if (m_modelEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Model is required."));
        m_modelEdit->setFocus();
        return false;
    }
    const QString srcs = m_dataSourcesEdit->toPlainText().trimmed();
    if (srcs.isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"),
                             tr("At least one data source is required."));
        m_dataSourcesEdit->setFocus();
        return false;
    }
    return true;
}

std::string CustomAircraftDialog::generateId() const {
    const std::string mfr  = slugify(m_manufacturerEdit->text().trimmed().toStdString());
    const std::string mdl  = slugify(m_modelEdit->text().trimmed().toStdString());
    const std::string var  = slugify(m_variantEdit->text().trimmed().toStdString());
    std::string id = "custom-" + mfr;
    if (!mdl.empty()) id += "-" + mdl;
    if (!var.empty()) id += "-" + var;
    return id;
}

void CustomAircraftDialog::onAccepted() {
    if (!validateFields()) {
        // Reopen the dialog by rejecting the acceptance — but QDialog::accept
        // has already been called by the button box. We need to block it.
        // This is called from QDialog::accept; just return without emitting.
        return;
    }

    const std::string id = generateId();

    // Build the library entry struct.
    arld::core::AircraftLibraryEntry entry;
    entry.id          = id;
    entry.displayName = m_nameEdit->text().trimmed().toStdString();
    entry.manufacturer= m_manufacturerEdit->text().trimmed().toStdString();
    entry.model       = m_modelEdit->text().trimmed().toStdString();
    entry.variant     = m_variantEdit->text().trimmed().toStdString();
    entry.category    = static_cast<arld::core::AircraftCategory>(
                            m_categoryCombo->currentData().toInt());
    entry.wingspanFt  = static_cast<float>(m_wingspanSpin->value());
    entry.lengthFt    = static_cast<float>(m_lengthSpin->value());
    entry.tailHeightFt= static_cast<float>(m_tailHeightSpin->value());
    if (m_propArcSpin->value() > 0.01)
        entry.propArcFt = static_cast<float>(m_propArcSpin->value());
    entry.defaultDisplayType = static_cast<arld::core::DisplayType>(
                                   m_displayTypeCombo->currentData().toInt());

    // SVG filename (will be saved separately)
    const std::string svgFilename = id + ".svg";
    entry.silhouetteSvg = svgFilename;

    // Data sources
    for (const QString& line : m_dataSourcesEdit->toPlainText().split('\n')) {
        const QString t = line.trimmed();
        if (!t.isEmpty())
            entry.dataSources.push_back(t.toStdString());
    }

    // --- Persist to user library ---
    const QString libDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/user_library";
    const QString svgDir = libDir + "/silhouettes";
    QDir().mkpath(libDir);
    QDir().mkpath(svgDir);

    // Serialize JSON
    try {
        using json = nlohmann::json;
        json j;
        j["schema_version"]        = 1;
        j["id"]                    = entry.id;
        j["display_name"]          = entry.displayName;
        j["manufacturer"]          = entry.manufacturer;
        j["model"]                 = entry.model;
        j["variant"]               = entry.variant;
        // category string
        switch (entry.category) {
            case arld::core::AircraftCategory::Warbird:         j["category"] = "WARBIRD"; break;
            case arld::core::AircraftCategory::JetFighter:      j["category"] = "JET_FIGHTER"; break;
            case arld::core::AircraftCategory::HeavyTransport:  j["category"] = "HEAVY_TRANSPORT"; break;
            case arld::core::AircraftCategory::Bomber:          j["category"] = "BOMBER"; break;
            case arld::core::AircraftCategory::GeneralAviation: j["category"] = "GENERAL_AVIATION"; break;
            case arld::core::AircraftCategory::Aerobatic:       j["category"] = "AEROBATIC"; break;
            case arld::core::AircraftCategory::Helicopter:      j["category"] = "HELICOPTER"; break;
            case arld::core::AircraftCategory::BusinessJet:     j["category"] = "BUSINESS_JET"; break;
        }
        j["wingspan_ft"]           = entry.wingspanFt;
        j["length_ft"]             = entry.lengthFt;
        j["tail_height_ft"]        = entry.tailHeightFt;
        if (entry.propArcFt)
            j["prop_arc_ft"]       = *entry.propArcFt;
        // display type string
        switch (entry.defaultDisplayType) {
            case arld::core::DisplayType::StaticDisplay:       j["default_display_type"] = "STATIC_DISPLAY"; break;
            case arld::core::DisplayType::WarbirdHeritage:     j["default_display_type"] = "WARBIRD_HERITAGE"; break;
            case arld::core::DisplayType::TaxiOnly:            j["default_display_type"] = "TAXI_ONLY"; break;
            case arld::core::DisplayType::MilitaryStatic:      j["default_display_type"] = "MILITARY_STATIC"; break;
            case arld::core::DisplayType::HotRamp:             j["default_display_type"] = "HOT_RAMP"; break;
            case arld::core::DisplayType::MediaPhotoPlatform:  j["default_display_type"] = "MEDIA_PHOTO_PLATFORM"; break;
            case arld::core::DisplayType::RampShow:            j["default_display_type"] = "RAMP_SHOW"; break;
        }
        j["silhouette_svg"]        = svgFilename;
        j["data_sources"]          = entry.dataSources;
        j["verified_date"]         = todayIso().toStdString();

        QFile jsonFile(libDir + "/" + QString::fromStdString(id) + ".json");
        if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&jsonFile);
            ts << QString::fromStdString(j.dump(2));
        }
    } catch (...) {
        // If JSON serialization fails, still emit (in-memory use is fine).
    }

    // Save SVG (if provided)
    if (!m_sanitizedSvg.empty()) {
        QFile svgFile(svgDir + "/" + QString::fromStdString(svgFilename));
        if (svgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&svgFile);
            ts << QString::fromStdString(m_sanitizedSvg);
        }
        // Update the entry's silhouetteSvg to the absolute path for immediate use
        // (the user library is not in Qt resources, so we store the absolute path).
        entry.silhouetteSvg = (svgDir + "/" + QString::fromStdString(svgFilename)).toStdString();
    }

    emit entryCreated(entry);
}

} // namespace arld::ui
