#include "sensor.h"

#include <Wire.h>

#include "usb_classes.h"

#define PRO_PCB

#ifdef PRO_FEATURES
#ifdef PRO_PCB
#define I2C_SCL 16
#define I2C_SDA 21
#else
#define I2C_SDA 43
#define I2C_SCL 44
#endif
#else
#define I2C_SDA 5
#define I2C_SCL 6
#define IMU_NRST 44
#endif

#define ROT_VECTOR_TYPE SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR

BNO08x BNO086::imu;

bool BNO086::init() {
#ifndef PRO_FEATURES
    pinMode(IMU_NRST, OUTPUT);
    digitalWrite(IMU_NRST, LOW);
    delay(100);
    digitalWrite(IMU_NRST, HIGH);
#endif
    if (!Wire.begin(I2C_SDA, I2C_SCL)) {
        // USBSerial.println("Could not initialize I2C");
        return false;
    }
    if(!imu.begin()) {
        // USBSerial.println("Could not communicate with IMU");
        return false;
    }
    if (!imu.setCalibrationConfig(SH2_CAL_ACCEL | SH2_CAL_MAG)) {
        // USBSerial.println("Could not configure IMU calibration");
        return false;
    }
    delay(100);
    if (!imu.enableReport(ROT_VECTOR_TYPE)) {
        // USBSerial.println("Could not enable IMU rotation vector");
        return false;
    }
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