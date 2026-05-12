#pragma once

// Application version string (Sprint 2-3-9)
#define ARLD_VERSION_STRING "1.0.0"

namespace arld::core {

// FAA CoW default clearance distances — all values in feet (TRD-ARCH-004)
inline constexpr float kStaticDisplayWingtip     = 25.0f;
inline constexpr float kWarbirdPropArcAddition   = 10.0f;
inline constexpr float kTaxiOnlyCorridor         = 50.0f;
inline constexpr float kMilitaryStaticStandoff   = 50.0f;
inline constexpr float kHotRampNoSmoking         = 100.0f;
inline constexpr float kRampShowCrowdLine        = 200.0f;
inline constexpr float kMediaPlatformBarrier     = 15.0f;

// Canvas defaults
inline constexpr float kDefaultGridSpacingFt     = 5.0f;
inline constexpr float kMinGridSpacingFt         = 1.0f;
inline constexpr float kMaxGridSpacingFt         = 25.0f;
inline constexpr int   kUndoHistoryDepth         = 100;
inline constexpr int   kAutoSaveIntervalSeconds  = 60;

// Zoom range (scale denominator: 1:200 to 1:5000)
inline constexpr int   kMinScaleDenominator      = 200;
inline constexpr int   kMaxScaleDenominator      = 5000;

// Performance
inline constexpr int   kTargetFps                = 60;
inline constexpr int   kMaxAircraftFullFps       = 200;

// Clearance zone fill opacity (SVG / QPainter)
inline constexpr float kAdvisoryZoneOpacity      = 0.25f;
inline constexpr float kViolationZoneOpacity     = 0.40f;

// Gear-state clearance addition
inline constexpr float kGearExtendedAdditionFt   = 8.0f;  // extra clearance when gear down
inline constexpr float kTailSwingAdvisoryFactor   = 1.2f; // advisory within 20% of tail-swing zone

// Metric conversion — applied at render time only; never stored in project files
inline constexpr double kFeetToMeters            = 0.3048;

} // namespace arld::core
