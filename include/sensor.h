#pragma once

#include <Arduino.h>
#include "SparkFun_BNO08x_Arduino_Library.h"

struct Orientation {
    float roll;
    float pitch;
    float yaw;
};

namespace BNO086 {

extern BNO08x imu;

bool init();

const Orientation& poll();

}   // namespace BNO086