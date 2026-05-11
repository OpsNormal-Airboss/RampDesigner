#include <arld/core/ClearanceRuleSet.h>

namespace arld::core {

ClearanceRuleSet ClearanceRuleSet::faaCoW() {
    // Default-constructed values already match FAA CoW.
    return ClearanceRuleSet{};
}

ClearanceRuleSet ClearanceRuleSet::icas() {
    ClearanceRuleSet rs;
    rs.rulesetId              = "icas_standard";
    rs.displayName            = "ICAS Safety Standard";
    rs.staticDisplayWingtipFt   = 30.0f;
    rs.warbirdPropArcBonusFt    = 40.0f;
    rs.taxiOnlyCorridorFt       = 50.0f;
    rs.militaryStaticStandoffFt = 60.0f;
    rs.hotRampStandoffFt        = 100.0f;
    rs.mediaPhotoPlatformFt     = 15.0f;
    rs.rampShowCrowdLineFt      = 200.0f;
    return rs;
}

} // namespace arld::core
