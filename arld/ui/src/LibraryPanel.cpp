#include <arld/ui/LibraryPanel.h>
#include <arld/ui/CustomAircraftDialog.h>
#include <arld/core/AircraftLibraryParser.h>
#include <arld/core/UnitConverter.h>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QWidget>

namespace arld::ui {

// ---------------------------------------------------------------------------
// DraggableListWidget
// ---------------------------------------------------------------------------
class DraggableListWidget : public QListWidget {
public:
    explicit DraggableListWidget(QWidget* parent = nullptr)
        : QListWidget(parent) {
        setDragEnabled(true);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setIconSize(QSize(48, 48));
        setSpacing(2);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        m_dragStartPos = event->pos();
        QListWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!(event->buttons() & Qt::LeftButton)) return;
        if ((event->pos() - m_dragStartPos).manhattanLength() < 6) return;

        QListWidgetItem* item = currentItem();
        if (!item) return;

        auto* mime = new QMimeData();
        mime->setData("application/x-arld-aircraft-id",
                      item->data(Qt::UserRole).toString().toUtf8());

        auto* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::CopyAction);
    }

private:
    QPoint m_dragStartPos;
};

// ---------------------------------------------------------------------------
// SVG thumbnail helper
// ---------------------------------------------------------------------------

static QPixmap renderSvgThumbnail(const QString& svgPath, int size) {
    QPixmap px(size, size);
    px.fill(Qt::transparent);

    QSvgRenderer renderer(svgPath);
    if (renderer.isValid()) {
        QPainter painter(&px);
        renderer.render(&painter, QRectF(0, 0, size, size));
    }
    return px;
}

// Render SVG from bytes (for user library SVGs not in Qt resources).
static QPixmap renderSvgByteThumbnail(const QByteArray& data, int size) {
    QPixmap px(size, size);
    px.fill(Qt::transparent);

    QSvgRenderer renderer(data);
    if (renderer.isValid()) {
        QPainter painter(&px);
        renderer.render(&painter, QRectF(0, 0, size, size));
    }
    return px;
}

// ---------------------------------------------------------------------------
// Category filter helpers
// ---------------------------------------------------------------------------

static int categoryIndex(arld::core::AircraftCategory cat) {
    switch (cat) {
        case arld::core::AircraftCategory::Warbird:         return 1;
        case arld::core::AircraftCategory::JetFighter:      return 2;
        case arld::core::AircraftCategory::Bomber:          return 3;
        case arld::core::AircraftCategory::HeavyTransport:  return 4;
        case arld::core::AircraftCategory::GeneralAviation: return 5;
        case arld::core::AircraftCategory::Aerobatic:       return 6;
        case arld::core::AircraftCategory::Helicopter:      return 7;
        case arld::core::AircraftCategory::BusinessJet:     return 8;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// LibraryPanel
// ---------------------------------------------------------------------------

LibraryPanel::LibraryPanel(QWidget* parent)
    : QDockWidget(tr("Aircraft Library"), parent) {

    auto* container = new QWidget(this);
    auto* vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(4, 4, 4, 4);
    vbox->setSpacing(4);

    // Search box
    m_searchEdit = new QLineEdit(container);
    m_searchEdit->setPlaceholderText(tr("Search aircraft..."));
    m_searchEdit->setClearButtonEnabled(true);
    vbox->addWidget(m_searchEdit);

    // Category filter
    m_categoryCombo = new QComboBox(container);
    m_categoryCombo->addItem(tr("All Categories"),     0);
    m_categoryCombo->addItem(tr("Warbird"),            1);
    m_categoryCombo->addItem(tr("Jet Fighter"),        2);
    m_categoryCombo->addItem(tr("Bomber"),             3);
    m_categoryCombo->addItem(tr("Heavy Transport"),    4);
    m_categoryCombo->addItem(tr("General Aviation"),   5);
    m_categoryCombo->addItem(tr("Aerobatic"),          6);
    m_categoryCombo->addItem(tr("Helicopter"),         7);
    m_categoryCombo->addItem(tr("Business Jet"),       8);
    vbox->addWidget(m_categoryCombo);

    // Aircraft list
    m_list = new DraggableListWidget(container);
    vbox->addWidget(m_list, 1);

    // Add Custom Aircraft button
    m_addCustomBtn = new QPushButton(tr("Add Custom Aircraft..."), container);
    vbox->addWidget(m_addCustomBtn);

    setWidget(container);
    setMinimumWidth(220);

    // Connect filter signals
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &LibraryPanel::onFilterChanged);
    connect(m_categoryCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &LibraryPanel::onFilterChanged);
    connect(m_addCustomBtn, &QPushButton::clicked,
            this, &LibraryPanel::onAddCustomAircraft);

    loadLibrary();
}

void LibraryPanel::loadLibrary() {
    // --- Bundled library ---
    QFile manifest(":/library/library_manifest.json");
    if (manifest.open(QIODevice::ReadOnly)) {
        const auto ids = arld::core::AircraftLibraryParser::parseManifest(
            manifest.readAll().toStdString());

        for (const auto& id : ids) {
            QFile f(QString(":/library/%1.json").arg(QString::fromStdString(id)));
            if (!f.open(QIODevice::ReadOnly)) continue;
            try {
                auto entry = arld::core::AircraftLibraryParser::parseEntry(
                    f.readAll().toStdString());
                m_entries.push_back(entry);
            } catch (...) {}
        }
    }

    // --- User library: scan AppDataLocation/user_library/*.json ---
    const QString userLibDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/user_library";
    QDir userDir(userLibDir);
    if (userDir.exists()) {
        const auto jsonFiles = userDir.entryList({"*.json"}, QDir::Files);
        for (const QString& fname : jsonFiles) {
            QFile f(userLibDir + "/" + fname);
            if (!f.open(QIODevice::ReadOnly)) continue;
            try {
                auto entry = arld::core::AircraftLibraryParser::parseEntry(
                    f.readAll().toStdString());
                // Avoid duplicates (may have been added earlier in the session).
                bool dup = false;
                for (const auto& e : m_entries)
                    if (e.id == entry.id) { dup = true; break; }
                if (!dup) m_entries.push_back(entry);
            } catch (...) {}
        }
    }

    rebuildList();
}

void LibraryPanel::rebuildList() {
    m_list->clear();

    const QString searchText = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    const int     catFilter  = m_categoryCombo ? m_categoryCombo->currentData().toInt() : 0;

    for (const auto& entry : m_entries) {
        // Category filter
        if (catFilter != 0 && categoryIndex(entry.category) != catFilter)
            continue;

        // Text filter (case-insensitive substring of name + manufacturer)
        if (!searchText.isEmpty()) {
            const QString haystack = (QString::fromStdString(entry.displayName)
                                      + " "
                                      + QString::fromStdString(entry.manufacturer))
                                     .toLower();
            if (!haystack.contains(searchText.toLower()))
                continue;
        }

        // Create list item
        auto* item = new QListWidgetItem(m_list);
        item->setData(Qt::UserRole, QString::fromStdString(entry.id));

        // Thumbnail (48×48)
        QPixmap thumb;
        const QString svgId = QString::fromStdString(entry.silhouetteSvg);

        // Check if this is an absolute path (user library) or a Qt resource ID.
        if (!entry.silhouetteSvg.empty() && entry.silhouetteSvg[0] == '/') {
            // Absolute path — load from filesystem.
            QFile svgFile(svgId);
            if (svgFile.open(QIODevice::ReadOnly))
                thumb = renderSvgByteThumbnail(svgFile.readAll(), 48);
        } else if (!entry.silhouetteSvg.empty()) {
            thumb = renderSvgThumbnail(
                QString(":/library/silhouettes/%1").arg(svgId), 48);
        }

        if (!thumb.isNull())
            item->setIcon(QIcon(thumb));

        // Label: bold name + dimensions
        const auto& uc = arld::core::UnitConverter::instance();
        const QString dims = tr("%1 × %2")
            .arg(uc.toDisplay(entry.wingspanFt), 0, 'f', 1)
            .arg(uc.toDisplay(entry.lengthFt),   0, 'f', 1);

        item->setText(QString::fromStdString(entry.displayName) + "\n" + dims + " " + uc.suffix());
        item->setToolTip(tr("%1  |  %2\nWingspan %3 ft  ×  Length %4 ft")
                         .arg(QString::fromStdString(entry.displayName))
                         .arg(QString::fromStdString(entry.manufacturer))
                         .arg(entry.wingspanFt, 0, 'f', 1)
                         .arg(entry.lengthFt,   0, 'f', 1));
    }
}

void LibraryPanel::onFilterChanged() {
    rebuildList();
}

void LibraryPanel::onAddCustomAircraft() {
    auto* dlg = new CustomAircraftDialog(this);
    connect(dlg, &CustomAircraftDialog::entryCreated,
            this, [this](arld::core::AircraftLibraryEntry entry) {
        m_entries.push_back(std::move(entry));
        rebuildList();
    });
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

const arld::core::AircraftLibraryEntry*
LibraryPanel::entryById(const std::string& id) const {
    for (const auto& e : m_entries)
        if (e.id == id) return &e;
    return nullptr;
}

} // namespace arld::ui
