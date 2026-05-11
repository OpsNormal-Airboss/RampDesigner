#include <arld/core/SvgSanitizer.h>
#include <catch2/catch_test_macros.hpp>
#include <string>

using arld::core::SvgSanitizer;

// Helper: build SVG test strings using plain string literals to avoid
// raw-string delimiter collisions with attribute values containing )".
static std::string makeSvg(const std::string& body) {
    return "<svg xmlns=\"http://www.w3.org/2000/svg\">" + body + "</svg>";
}

TEST_CASE("SvgSanitizer strips <script> elements", "[svg_sanitizer]") {
    const std::string input = makeSvg(
        "<script>alert('xss');</script>"
        "<rect width=\"10\" height=\"10\"/>");

    const std::string result = SvgSanitizer::sanitize(input);

    REQUIRE(result.find("<script") == std::string::npos);
    REQUIRE(result.find("alert") == std::string::npos);
    // Safe content preserved
    REQUIRE(result.find("<rect") != std::string::npos);
}

TEST_CASE("SvgSanitizer strips <foreignObject> elements", "[svg_sanitizer]") {
    const std::string input = makeSvg(
        "<foreignObject width=\"100\" height=\"100\"><body>evil</body></foreignObject>"
        "<circle cx=\"5\" cy=\"5\" r=\"5\"/>");

    const std::string result = SvgSanitizer::sanitize(input);

    REQUIRE(result.find("<foreignObject") == std::string::npos);
    REQUIRE(result.find("evil") == std::string::npos);
    REQUIRE(result.find("<circle") != std::string::npos);
}

TEST_CASE("SvgSanitizer strips DOCTYPE with ENTITY (XXE)", "[svg_sanitizer]") {
    const std::string input =
        "<?xml version=\"1.0\"?>"
        "<!DOCTYPE foo [<!ENTITY xxe SYSTEM \"file:///etc/passwd\">]>"
        + makeSvg("<text>&xxe;</text>");

    const std::string result = SvgSanitizer::sanitize(input);

    REQUIRE(result.find("<!DOCTYPE") == std::string::npos);
    REQUIRE(result.find("<!ENTITY") == std::string::npos);
    REQUIRE(result.find("etc/passwd") == std::string::npos);
}

TEST_CASE("SvgSanitizer strips on* event attributes", "[svg_sanitizer]") {
    // Build with a plain attribute value that has no special raw-literal chars.
    const std::string input = makeSvg(
        "<rect width=\"100\" height=\"50\" onclick=\"doEvil\" onmouseover=\"doMore\"/>");

    const std::string result = SvgSanitizer::sanitize(input);

    REQUIRE(result.find("onclick") == std::string::npos);
    REQUIRE(result.find("onmouseover") == std::string::npos);
    // Rect element itself preserved
    REQUIRE(result.find("<rect") != std::string::npos);
}

TEST_CASE("SvgSanitizer strips javascript: href values", "[svg_sanitizer]") {
    const std::string input = makeSvg(
        "<a href=\"javascript:void0\">click</a>");

    const std::string result = SvgSanitizer::sanitize(input);

    REQUIRE(result.find("javascript:") == std::string::npos);
    // The anchor text content can remain
    REQUIRE(result.find("click") != std::string::npos);
}

TEST_CASE("SvgSanitizer passes clean SVG unchanged", "[svg_sanitizer]") {
    const std::string input =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 50\">"
        "<rect width=\"100\" height=\"50\" fill=\"#4a4a4a\"/>"
        "</svg>";

    const std::string result = SvgSanitizer::sanitize(input);

    REQUIRE(result == input);
}

TEST_CASE("SvgSanitizer strips multiline script block", "[svg_sanitizer]") {
    const std::string input =
        "<svg xmlns=\"http://www.w3.org/2000/svg\">\n"
        "<script type=\"text/javascript\">\n"
        "  function evil() {\n"
        "    document.cookie = 'stolen';\n"
        "  }\n"
        "</script>\n"
        "<rect width=\"10\" height=\"10\"/>\n"
        "</svg>";

    const std::string result = SvgSanitizer::sanitize(input);

    REQUIRE(result.find("<script") == std::string::npos);
    REQUIRE(result.find("document.cookie") == std::string::npos);
    REQUIRE(result.find("<rect") != std::string::npos);
}
