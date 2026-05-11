#pragma once
#include <arld/core/AircraftLibraryEntry.h>
#include <string>
#include <vector>

namespace arld::core {

class AircraftLibraryParser {
public:
    // Parse a single aircraft entry from a UTF-8 JSON string.
    static AircraftLibraryEntry parseEntry(const std::string& jsonContent);

    // Parse library_manifest.json and return entry IDs in display order.
    static std::vector<std::string> parseManifest(const std::string& jsonContent);
};

} // namespace arld::core
