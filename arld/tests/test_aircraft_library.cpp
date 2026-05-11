#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <arld/core/AircraftLibraryParser.h>

using namespace arld::core;
using Catch::Matchers::WithinRel;

TEST_CASE("AircraftLibraryParser - parse valid P-51D entry", "[library]") {
    const std::string json = R"({
        "schema_version": 1,
        "id": "north-american-p51-d",
        "display_name": "P-51D Mustang",
        "manufacturer": "North American Aviation",
        "model": "P-51",
        "variant": "D",
        "category": "WARBIRD",
        "wingspan_ft": 37.0,
        "length_ft": 32.3,
        "tail_height_ft": 13.8,
        "prop_arc_ft": 11.2,
        "default_display_type": "WARBIRD_HERITAGE",
        "silhouette_svg": "north-american-p51-d.svg",
        "data_sources": ["Source A", "Source B", "Source C"]
    })";

    const auto e = AircraftLibraryParser::parseEntry(json);

    REQUIRE(e.id           == "north-american-p51-d");
    REQUIRE(e.displayName  == "P-51D Mustang");
    REQUIRE(e.manufacturer == "North American Aviation");
    REQUIRE(e.variant      == "D");
    REQUIRE(e.category     == AircraftCategory::Warbird);
    REQUIRE_THAT(e.wingspanFt, WithinRel(37.0f, 0.001f));
    REQUIRE_THAT(e.lengthFt,   WithinRel(32.3f, 0.001f));
    REQUIRE(e.propArcFt.has_value());
    REQUIRE_THAT(*e.propArcFt, WithinRel(11.2f, 0.001f));
    REQUIRE_FALSE(e.rotorDiameterFt.has_value());
    REQUIRE(e.defaultDisplayType == DisplayType::WarbirdHeritage);
    REQUIRE(e.silhouetteSvg == "north-american-p51-d.svg");
    REQUIRE(e.dataSources.size() == 3);
}

TEST_CASE("AircraftLibraryParser - parse jet entry without prop_arc", "[library]") {
    const std::string json = R"({
        "schema_version": 1,
        "id": "general-dynamics-f16-a",
        "display_name": "F-16A Fighting Falcon",
        "manufacturer": "General Dynamics",
        "model": "F-16",
        "variant": "A",
        "category": "JET_FIGHTER",
        "wingspan_ft": 31.0,
        "length_ft": 49.3,
        "tail_height_ft": 16.8,
        "default_display_type": "MILITARY_STATIC",
        "silhouette_svg": "general-dynamics-f16-a.svg",
        "data_sources": ["Source A"]
    })";

    const auto e = AircraftLibraryParser::parseEntry(json);

    REQUIRE(e.id       == "general-dynamics-f16-a");
    REQUIRE(e.category == AircraftCategory::JetFighter);
    REQUIRE_FALSE(e.propArcFt.has_value());
    REQUIRE(e.defaultDisplayType == DisplayType::MilitaryStatic);
}

TEST_CASE("AircraftLibraryParser - parse helicopter with rotor_diameter", "[library]") {
    const std::string json = R"({
        "schema_version": 1,
        "id": "bell-oh58-d",
        "display_name": "OH-58D Kiowa Warrior",
        "manufacturer": "Bell Helicopter",
        "model": "OH-58",
        "variant": "D",
        "category": "HELICOPTER",
        "wingspan_ft": 35.0,
        "length_ft": 40.9,
        "tail_height_ft": 12.5,
        "rotor_diameter_ft": 35.0,
        "default_display_type": "MILITARY_STATIC",
        "silhouette_svg": "bell-oh58-d.svg",
        "data_sources": ["Source A"]
    })";

    const auto e = AircraftLibraryParser::parseEntry(json);

    REQUIRE(e.category == AircraftCategory::Helicopter);
    REQUIRE(e.rotorDiameterFt.has_value());
    REQUIRE_THAT(*e.rotorDiameterFt, WithinRel(35.0f, 0.001f));
    REQUIRE_FALSE(e.propArcFt.has_value());
}

TEST_CASE("AircraftLibraryParser - parse manifest returns ordered IDs", "[library]") {
    const std::string json = R"({
        "schema_version": 1,
        "entries": [
            "north-american-p51-d",
            "general-dynamics-f16-a",
            "bell-oh58-d"
        ]
    })";

    const auto ids = AircraftLibraryParser::parseManifest(json);

    REQUIRE(ids.size() == 3);
    REQUIRE(ids[0] == "north-american-p51-d");
    REQUIRE(ids[1] == "general-dynamics-f16-a");
    REQUIRE(ids[2] == "bell-oh58-d");
}

TEST_CASE("AircraftLibraryParser - invalid JSON throws", "[library]") {
    REQUIRE_THROWS(AircraftLibraryParser::parseEntry("{bad json}"));
}

TEST_CASE("AircraftLibraryParser - unknown category throws", "[library]") {
    const std::string json = R"({
        "schema_version": 1,
        "id": "test",
        "display_name": "Test",
        "manufacturer": "Test",
        "model": "T",
        "category": "BIPLANE",
        "wingspan_ft": 20.0,
        "length_ft": 18.0,
        "tail_height_ft": 6.0,
        "default_display_type": "STATIC_DISPLAY",
        "silhouette_svg": "test.svg",
        "data_sources": ["Source"]
    })";

    REQUIRE_THROWS(AircraftLibraryParser::parseEntry(json));
}

TEST_CASE("AircraftLibraryParser - variant field is optional", "[library]") {
    const std::string json = R"({
        "schema_version": 1,
        "id": "test-aircraft-x",
        "display_name": "Test Aircraft",
        "manufacturer": "Test Corp",
        "model": "X",
        "category": "GENERAL_AVIATION",
        "wingspan_ft": 36.0,
        "length_ft": 27.0,
        "tail_height_ft": 9.0,
        "default_display_type": "STATIC_DISPLAY",
        "silhouette_svg": "test-aircraft-x.svg",
        "data_sources": ["Source A"]
    })";

    const auto e = AircraftLibraryParser::parseEntry(json);
    REQUIRE(e.variant.empty());
    REQUIRE(e.category == AircraftCategory::GeneralAviation);
}
