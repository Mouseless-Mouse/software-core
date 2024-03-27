#pragma once

#include <Wire.h>
#include "SparkFun_BNO08x_Arduino_Library.h"

#define BNO08X_ADDR 0x4B


//Struct for rotation measurements
struct Orientation
{
    float roll;
    float pitch;
    float yaw;
};


//Sensor Class
namespace BNO086 {

    extern BNO08x imu;

    //Initializes the sensor
    bool init();

    //returns the data readings
    Orientation& poll();

} // namespace BNO086