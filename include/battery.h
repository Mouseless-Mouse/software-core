#include <Arduino.h>

#ifdef PRO_FEATURES
#define BAT_ADC_PIN GPIO_NUM_4
#endif

#define BATTERY_FILTER_GAIN 0.3f

namespace battery
{
    float get_voltage()
    {
#ifdef PRO_FEATURES
        static float voltage = 0.0f;
        float raw = static_cast<float>(analogReadMilliVolts(BAT_ADC_PIN)) / 1000.0f;
        voltage = voltage * BATTERY_FILTER_GAIN + (1.0f - BATTERY_FILTER_GAIN) * raw;
        return voltage;
#else
        return 3.7f;
#endif
    }

    float get_level()
    {
        auto raw = (get_voltage() - 3.2f) * 200.0f;
        if (raw < 0.0f)
        {
            return 0.0f;
        }
        else if (raw > 100.0f)
        {
            return 100.0f;
        }
        else
        {
            return raw;
        }
    }
}