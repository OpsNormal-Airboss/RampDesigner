#include <arld/core/SvgSanitizer.h>
#include <algorithm>
#include <cctype>
#include <regex>
#include <string>

namespace arld::core {

namespace {

// Case-insensitive string search helper.
static bool icontains(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(),
                          needle.begin(),  needle.end(),
                          [](unsigned char a, unsigned char b) {
                              return std::tolower(a) == std::tolower(b);
                          });
    return it != haystack.end();
}

// Remove all occurrences of a regex (case-insensitive) from @p s.
static std::string removeAll(const std::string& s, const std::string& pattern) {
    try {
        std::regex re(pattern, std::regex::icase | std::regex::ECMAScript);
        return std::regex_replace(s, re, "");
    } catch (const std::regex_error&) {
        return s; // if pattern is bad (shouldn't happen), return unchanged
    }
}

} // anonymous namespace

std::string SvgSanitizer::sanitize(const std::string& svgInput) {
    std::string result = svgInput;

    // -----------------------------------------------------------------------
    // 1. Strip <script> elements (including multiline, attributes, self-close)
    // -----------------------------------------------------------------------
    result = removeAll(result,
        R"(<script[\s\S]*?(?:</script\s*>|/>))");

    // -----------------------------------------------------------------------
    // 2. Strip <foreignObject> elements
    // -----------------------------------------------------------------------
    result = removeAll(result,
        R"(<foreignObject[\s\S]*?(?:</foreignObject\s*>|/>))");

    // -----------------------------------------------------------------------
    // 3. Strip <!DOCTYPE ...> declarations (XXE / ENTITY injection)
    //    Also strip inline <!ENTITY declarations.
    // -----------------------------------------------------------------------
    result = removeAll(result,
        R"(<!DOCTYPE\b[\s\S]*?>)");
    result = removeAll(result,
        R"(<!ENTITY\b[\s\S]*?>)");

    // -----------------------------------------------------------------------
    // 4. Strip on* event attributes: on[a-z]+ = "..." or on[a-z]+ = '...'
    // -----------------------------------------------------------------------
    result = removeAll(result,
        R"(\s+on[a-z][a-zA-Z0-9]*\s*=\s*(?:"[^"]*"|'[^']*'))");

    // -----------------------------------------------------------------------
    // 5. Strip href="javascript:..." and xlink:href="javascript:..."
    //    (replace value with empty string to preserve attribute structure)
    // -----------------------------------------------------------------------
    result = removeAll(result,
        R"((?:xlink:)?href\s*=\s*"javascript:[^"]*")");
    result = removeAll(result,
        R"((?:xlink:)?href\s*=\s*'javascript:[^']*')");

    return result;
}

} // namespace arld::core
