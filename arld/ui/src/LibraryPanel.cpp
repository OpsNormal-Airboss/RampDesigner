#include <arld/ui/LibraryPanel.h>
#include <arld/core/AircraftLibraryParser.h>
#include <QFile>
#include <QListWidget>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>

namespace arld::ui {

// ---------------------------------------------------------------------------
// DraggableListWidget — initiates a drag with aircraft ID as MIME payload
// ---------------------------------------------------------------------------
class DraggableListWidget : public QListWidget {
public:
    explicit DraggableListWidget(QWidget* parent = nullptr)
        : QListWidget(parent) {
        setDragEnabled(true);
        setSelectionMode(QAbstractItemView::SingleSelection);
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
// LibraryPanel
// ---------------------------------------------------------------------------
LibraryPanel::LibraryPanel(QWidget* parent)
    : QDockWidget(tr("Aircraft Library"), parent) {
    m_list = new DraggableListWidget(this);
    setWidget(m_list);
    setMinimumWidth(200);
    loadLibrary();
}

void LibraryPanel::loadLibrary() {
    QFile manifest(":/library/library_manifest.json");
    if (!manifest.open(QIODevice::ReadOnly)) return;
    const auto ids = arld::core::AircraftLibraryParser::parseManifest(
        manifest.readAll().toStdString());

    for (const auto& id : ids) {
        QFile f(QString(":/library/%1.json").arg(QString::fromStdString(id)));
        if (!f.open(QIODevice::ReadOnly)) continue;

        try {
            auto entry = arld::core::AircraftLibraryParser::parseEntry(
                f.readAll().toStdString());
            m_entries.push_back(entry);

            auto* item = new QListWidgetItem(
                QString::fromStdString(entry.displayName), m_list);
            item->setData(Qt::UserRole, QString::fromStdString(entry.id));
            item->setToolTip(tr("%1 ft × %2 ft  (wingspan × length)")
                .arg(entry.wingspanFt, 0, 'f', 1)
                .arg(entry.lengthFt,   0, 'f', 1));
        } catch (...) {
            // Bad JSON entry: skip silently in PoC.
        }
    }
}

const arld::core::AircraftLibraryEntry*
LibraryPanel::entryById(const std::string& id) const {
    for (const auto& e : m_entries)
        if (e.id == id) return &e;
    return nullptr;
}

} // namespace arld::ui
