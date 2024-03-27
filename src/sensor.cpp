#include "sensor.h"

#include <Wire.h>

#ifdef PRO_FEATURES
#define I2C_SDA 43
#define I2C_SCL 44
#else
#define I2C_SDA 5
#define I2C_SCL 6
#endif

BNO08x BNO086::imu;

bool BNO086::init() {
    if (!Wire.begin(I2C_SDA, I2C_SCL)) {
        Serial.println("Error beginning Wire");
        return false;
    }
    if (!imu.begin(0x4B, Wire, -1, -1)) {
        Serial.println("Error beginning IMU connection");
        return false;
    }
    if (!imu.enableGeomagneticRotationVector()) {
        Serial.println("Error enabling reports");
        return false;
    }
    return true;
}

const Orientation& BNO086::poll() {
    static Orientation current{0, 0, 0};
    if (imu.wasReset()) {
        imu.enableGeomagneticRotationVector();
    }
    if (imu.getSensorEvent())
        if (imu.getSensorEventID() == SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR)
            current = Orientation{imu.getRoll(), imu.getPitch(), imu.getYaw()};
    
    return current;
}