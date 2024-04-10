#pragma once

#include <Wire.h>
#include "SparkFun_BNO08x_Arduino_Library.h"

#define BNO08X_ADDR 0x4B


// Rotation measurement structure
struct Orientation
{
    float roll;
    float pitch;
    float yaw;
};


// Sensor interface
namespace BNO086 {

    extern BNO08x imu;

    // Initialize the sensor
    bool init();

    // Poll the sensor for new data
    Orientation& poll();

} // namespace BNO086