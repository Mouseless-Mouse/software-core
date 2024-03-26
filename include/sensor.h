#pragma once

#include <Wire.h>
#include "SparkFun_BNO08x_Arduino_Library.h"

#define BNO08X_ADDR 0x4B


//Struct for rotation measurements
struct rData
{
    float roll;
    float pitch;
    float yaw;
};


//Sensor Class
class Sensor : public BNO08x {
public:
    Sensor() : BNO08x() {}

    //Initializes the sensor
    void init(){
        Wire.begin(43,44);

        //Checks i2c
        if(begin(BNO08X_ADDR,Wire,-1-1) == false){
            Serial.println("BNO08x not detected at default I2C address.");
            while (1);
        }
        Serial.println("BNO08x found!");

        //checks geo sensor
        if (enableGeomagneticRotationVector() == true) {
            Serial.println(F("Geomagnetic Rotation vector enabled"));
        } 
        else {
            Serial.println("Could not enable geomagnetic rotation vector");
        }

        Serial.println("Reading events");
        delay(100);
    }


    //returns the data readings
    rData getData(){

        rData data;

        // Has a new event come in on the Sensor Hub Bus?
        if (getSensorEvent() == true) {

            // is it the correct sensor data we want?
            if (getSensorEventID() == SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR) {

                data.roll = (getRoll()) * 180.0 / PI; // Convert roll to degrees
                data.pitch = (getPitch()) * 180.0 / PI; // Convert pitch to degrees
                data.yaw = (getYaw()) * 180.0 / PI; // Convert yaw / heading to degrees
            }
        }
        return data;
    }
};




