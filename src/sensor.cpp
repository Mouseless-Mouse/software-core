#include "sensor.h"

#include <Wire.h>

#ifdef PRO_FEATURES
#define I2C_SDA 43
#define I2C_SCL 44
#else
#define I2C_SDA 5
#define I2C_SCL 6
#endif

#define ROT_VECTOR_TYPE SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR

BNO08x BNO086::imu;

bool BNO086::init() {
    if (!Wire.begin(43,44))
        return false;
    if(!imu.begin())
        return false;
    if (!imu.setCalibrationConfig(SH2_CAL_ACCEL | SH2_CAL_MAG))
        return false;
    delay(100);
    if (!imu.enableReport(ROT_VECTOR_TYPE))
        return false;
    delay(100);
    return true;
}

Orientation& BNO086::poll() {
    static Orientation data{0, 0, 0};

    // Has a new event come in on the Sensor Hub Bus?
    if (imu.getSensorEvent() && imu.getSensorEventID() == ROT_VECTOR_TYPE) {
        data.roll = (imu.getRoll()) * 180.0 / PI; // Convert roll to degrees
        data.pitch = (imu.getPitch()) * 180.0 / PI; // Convert pitch to degrees
        data.yaw = (imu.getYaw()) * 180.0 / PI; // Convert yaw / heading to degrees
    }
    return data;
}