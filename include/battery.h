#pragma once
#include <Arduino.h>

#ifdef PRO_FEATURES
#define BAT_ADC_PIN GPIO_NUM_4
#endif

#define BATTERY_FILTER_GAIN 0.3f

namespace battery {

/// @brief Get current battery voltage, if available. If not, return a default
/// voltage of 3.7V. Filtered using an alpha filter with gain of
/// BATTERY_FILTER_GAIN.
/// @return The current battery voltage.
float get_voltage();
/// @brief Get the current estimated battery percentage. Returns 100% if the
/// battery voltage is not available; otherwise, assumes that the battery's
/// voltage-charge relationship is linear and that it is fully charged at 3.7V
/// and empty at 3.2V. Clamps the result to the range 0% to 100%.
/// @return The current estimated battery percentage.
float get_level();

} // namespace battery