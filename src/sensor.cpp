#include "sensor.h"

#include <Wire.h>

#include "state.h"
#include "usb_classes.h"


// #define PRO_PCB

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

#define FLOAT_LIT_IMPL(d) d##f
#define FLOAT_LIT(d) FLOAT_LIT_IMPL(d)
#define PI_FLOAT FLOAT_LIT(PI)

BNO08x BNO086::imu;

bool BNO086::init(bool wireInitialized) {
#ifndef PRO_FEATURES
    pinMode(IMU_NRST, OUTPUT);
    digitalWrite(IMU_NRST, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(IMU_NRST, HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
#endif
    if (!wireInitialized && !Wire.begin(I2C_SDA, I2C_SCL)) {
        // USBSerial.println("Could not initialize I2C");
        return false;
    }
#ifdef PRO_FEATURES
    if (!imu.begin())
#else
    if (!imu.begin(BNO08X_ADDR) && !imu.begin(BNO08X_ADDR2))
#endif
    {
        // USBSerial.println("Could not communicate with IMU");
        return false;
    }
    if (!imu.setCalibrationConfig(SH2_CAL_ACCEL | SH2_CAL_MAG)) {
        // USBSerial.println("Could not configure IMU calibration");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    if (!imu.enableReport(ROT_VECTOR_TYPE)) {
        // USBSerial.println("Could not enable IMU rotation vector");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    return true;
}

Orientation BNO086::poll() {
    // Has a new event come in on the Sensor Hub Bus?
    uint8_t eventId;
    TickType_t lastReportTime = xTaskGetTickCount();
    do {
        while (!imu.getSensorEvent()) {
            if (imu.wasReset()) {
                imu.enableReport(ROT_VECTOR_TYPE);
            } else if (xTaskGetTickCount() - lastReportTime >
                       pdMS_TO_TICKS(500)) {
                Error<TaskLog>().println("IMU crash detected! Resetting...");
                lastReportTime = xTaskGetTickCount();
                init(true);
                vTaskDelay(pdMS_TO_TICKS(100));
            } else
                vTaskDelay(pdMS_TO_TICKS(20));
        }
        eventId = imu.getSensorEventID();
    } while (eventId != ROT_VECTOR_TYPE);

    return Orientation{
        imu.getRoll() * 180.f / PI_FLOAT,  // Convert roll to degrees
        imu.getPitch() * 180.f / PI_FLOAT, // Convert pitch to degrees
        imu.getYaw() * 180.f / PI_FLOAT    // Convert yaw / heading to degrees
    };
}