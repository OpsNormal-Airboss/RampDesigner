#pragma once
#include <string>

namespace arld::core {

/// Sanitizes SVG strings for safe embedding.
///
/// Strips:
///   - <script> ... </script> elements (and <script ... />)
///   - <foreignObject> ... </foreignObject> elements
///   - <!DOCTYPE> declarations containing SYSTEM or PUBLIC (XXE prevention)
///   - <!ENTITY declarations
///   - on* event attributes (onclick, onload, etc.)
///   - href attributes whose value starts with "javascript:"
///
/// A clean SVG passes through unchanged (aside from removal of the above).
class SvgSanitizer {
public:
    /// Returns a sanitized copy of @p svgInput.
    static std::string sanitize(const std::string& svgInput);
};

} // namespace arld::core
