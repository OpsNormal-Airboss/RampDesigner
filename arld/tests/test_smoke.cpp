#include <catch2/catch_test_macros.hpp>
#include <arld/core/Layout.h>
#include <arld/core/Config.h>

TEST_CASE("Layout constructs and destructs without error", "[smoke]") {
    arld::core::Layout layout;
    REQUIRE(true);
}

TEST_CASE("Config clearance constants match FAA CoW defaults", "[smoke][config]") {
    REQUIRE(arld::core::kStaticDisplayWingtip   == 25.0f);
    REQUIRE(arld::core::kWarbirdPropArcAddition == 10.0f);
    REQUIRE(arld::core::kTaxiOnlyCorridor       == 50.0f);
    REQUIRE(arld::core::kMilitaryStaticStandoff == 50.0f);
    REQUIRE(arld::core::kHotRampNoSmoking       == 100.0f);
    REQUIRE(arld::core::kRampShowCrowdLine      == 200.0f);
    REQUIRE(arld::core::kMediaPlatformBarrier   == 15.0f);
}

TEST_CASE("Config canvas defaults are within valid ranges", "[smoke][config]") {
    REQUIRE(arld::core::kDefaultGridSpacingFt >= arld::core::kMinGridSpacingFt);
    REQUIRE(arld::core::kDefaultGridSpacingFt <= arld::core::kMaxGridSpacingFt);
    REQUIRE(arld::core::kUndoHistoryDepth     >= 100);
    REQUIRE(arld::core::kMinScaleDenominator  == 200);
    REQUIRE(arld::core::kMaxScaleDenominator  == 5000);
}
