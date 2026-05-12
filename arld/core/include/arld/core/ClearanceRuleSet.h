#pragma once
#include <string>

namespace arld::core {

struct ClearanceRuleSet {
    std::string rulesetId   = "faa_cow";
    std::string displayName = "FAA Certificate of Waiver";

    float staticDisplayWingtipFt   = 25.0f;
    float warbirdPropArcBonusFt    = 35.0f;  // hull gap = propArcRadius + this
    float taxiOnlyCorridorFt       = 50.0f;
    float militaryStaticStandoffFt = 50.0f;
    float hotRampStandoffFt        = 100.0f;
    float mediaPhotoPlatformFt     = 15.0f;
    float rampShowCrowdLineFt      = 200.0f;

    static ClearanceRuleSet faaCoW();    // FAA Certificate of Waiver defaults
    static ClearanceRuleSet icas();      // ICAS Safety Standard

    // Value equality (ignores rulesetId/displayName — used to detect saves needed).
    bool sameValues(const ClearanceRuleSet& o) const {
        return staticDisplayWingtipFt   == o.staticDisplayWingtipFt
            && warbirdPropArcBonusFt    == o.warbirdPropArcBonusFt
            && taxiOnlyCorridorFt       == o.taxiOnlyCorridorFt
            && militaryStaticStandoffFt == o.militaryStaticStandoffFt
            && hotRampStandoffFt        == o.hotRampStandoffFt
            && mediaPhotoPlatformFt     == o.mediaPhotoPlatformFt
            && rampShowCrowdLineFt      == o.rampShowCrowdLineFt;
    }
};

} // namespace arld::core
