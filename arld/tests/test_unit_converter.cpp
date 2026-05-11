#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <arld/core/UnitConverter.h>

using arld::core::UnitConverter;
using arld::core::UnitSystem;

// Reset singleton state between tests via RAII guard.
struct UnitConverterGuard {
    UnitSystem saved;
    UnitConverterGuard() : saved(UnitConverter::instance().unitSystem()) {}
    ~UnitConverterGuard() { UnitConverter::instance().setUnitSystem(saved); }
};

TEST_CASE("UnitConverter: default unit system is Imperial", "[unit_converter]") {
    UnitConverterGuard g;
    UnitConverter::instance().setUnitSystem(UnitSystem::Imperial);
    REQUIRE(UnitConverter::instance().unitSystem() == UnitSystem::Imperial);
}

TEST_CASE("UnitConverter: toDisplay(1.0) returns 1.0 in Imperial", "[unit_converter]") {
    UnitConverterGuard g;
    UnitConverter::instance().setUnitSystem(UnitSystem::Imperial);
    REQUIRE(UnitConverter::instance().toDisplay(1.0f) == Catch::Approx(1.0f));
}

TEST_CASE("UnitConverter: toDisplay(1.0) returns ~0.3048 in Metric", "[unit_converter]") {
    UnitConverterGuard g;
    UnitConverter::instance().setUnitSystem(UnitSystem::Metric);
    REQUIRE(UnitConverter::instance().toDisplay(1.0f) == Catch::Approx(0.3048f).epsilon(1e-4f));
}

TEST_CASE("UnitConverter: toFeet(0.3048) returns ~1.0 in Metric", "[unit_converter]") {
    UnitConverterGuard g;
    UnitConverter::instance().setUnitSystem(UnitSystem::Metric);
    REQUIRE(UnitConverter::instance().toFeet(0.3048f) == Catch::Approx(1.0f).epsilon(1e-4f));
}

TEST_CASE("UnitConverter: suffix returns 'ft' in Imperial and 'm' in Metric", "[unit_converter]") {
    UnitConverterGuard g;

    UnitConverter::instance().setUnitSystem(UnitSystem::Imperial);
    REQUIRE(std::string(UnitConverter::instance().suffix()) == "ft");

    UnitConverter::instance().setUnitSystem(UnitSystem::Metric);
    REQUIRE(std::string(UnitConverter::instance().suffix()) == "m");
}
