#pragma once

namespace arld::core {

enum class UnitSystem { Imperial, Metric };

class UnitConverter {
public:
    static UnitConverter& instance();

    UnitSystem unitSystem() const { return m_system; }
    void setUnitSystem(UnitSystem s) { m_system = s; }

    /// Convert a value stored in feet to the current display unit (ft or m).
    float toDisplay(float valueFt) const;

    /// Convert a display-unit value back to feet.
    float toFeet(float displayValue) const;

    /// Returns "ft" or "m" depending on the active unit system.
    const char* suffix() const;

private:
    UnitConverter() = default;
    UnitSystem m_system = UnitSystem::Imperial;
};

} // namespace arld::core
