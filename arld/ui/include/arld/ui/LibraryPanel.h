#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <QDockWidget>
#include <vector>

class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace arld::ui {

class CustomAircraftDialog;

class LibraryPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget* parent = nullptr);

    const std::vector<arld::core::AircraftLibraryEntry>& entries() const { return m_entries; }

    // Returns the entry for the given aircraft id, or nullptr if not found.
    const arld::core::AircraftLibraryEntry* entryById(const std::string& id) const;

public slots:
    /// Reload all library entries from QRC + user data directory and rebuild the list.
    void reloadLibrary();

private slots:
    void onFilterChanged();
    void onAddCustomAircraft();
    void applySort();

private:
    void loadLibrary();
    void rebuildList();

    std::vector<arld::core::AircraftLibraryEntry> m_entries;
    QLineEdit*   m_searchEdit      = nullptr;
    QComboBox*   m_categoryCombo   = nullptr;
    QComboBox*   m_sortCombo       = nullptr;
    QListWidget* m_list            = nullptr;
    QPushButton* m_addCustomBtn    = nullptr;
};

} // namespace arld::ui
