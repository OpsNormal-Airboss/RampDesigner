#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <QDockWidget>
#include <vector>

class QListWidget;

namespace arld::ui {

class LibraryPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget* parent = nullptr);

    const std::vector<arld::core::AircraftLibraryEntry>& entries() const { return m_entries; }

    // Returns the entry for the given aircraft id, or nullptr if not found.
    const arld::core::AircraftLibraryEntry* entryById(const std::string& id) const;

private:
    void loadLibrary();

    std::vector<arld::core::AircraftLibraryEntry> m_entries;
    QListWidget* m_list = nullptr;
};

} // namespace arld::ui
