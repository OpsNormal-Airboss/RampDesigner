#include <arld/core/UnitConverter.h>

namespace arld::core {

static constexpr float kFeetToMetres = 0.3048f;
static constexpr float kMetresToFeet = 1.0f / kFeetToMetres;

UnitConverter& UnitConverter::instance() {
    static UnitConverter s_instance;
    return s_instance;
}

float UnitConverter::toDisplay(float valueFt) const {
    if (m_system == UnitSystem::Metric) {
        return valueFt * kFeetToMetres;
    }
    return valueFt;
}

float UnitConverter::toFeet(float displayValue) const {
    if (m_system == UnitSystem::Metric) {
        return displayValue * kMetresToFeet;
    }
    return displayValue;
}

const char* UnitConverter::suffix() const {
    return (m_system == UnitSystem::Metric) ? "m" : "ft";
}

} // namespace arld::core
