
// #include <Arduino.h>

// #include <Wire.h>



// void setReports(void);

// #include "SparkFun_BNO08x_Arduino_Library.h"  // CTRL+Click here to get the library: http://librarymanager/All#SparkFun_BNO08x
// BNO08x myIMU;

// // For the most reliable interaction with the SHTP bus, we need
// // to use hardware reset control, and to monitor the H_INT pin.
// // The H_INT pin will go low when its okay to talk on the SHTP bus.
// // Note, these can be other GPIO if you like.
// // Define as -1 to disable these features.
// #define BNO08X_INT  A4
// //#define BNO08X_INT  -1
// #define BNO08X_RST  A5
// //#define BNO08X_RST  -1

// #define BNO08X_ADDR 0x4B  // SparkFun BNO08x Breakout (Qwiic) defaults to 0x4B
// //#define BNO08X_ADDR 0x4A // Alternate address if ADR jumper is closed

// void setup() {
//   Serial.begin(115200);
  
//   while(!Serial) delay(10); // Wait for Serial to become available.
//   // Necessary for boards with native USB (like the SAMD51 Thing+).
//   // For a final version of a project that does not need serial debug (or a USB cable plugged in),
//   // Comment out this while loop, or it will prevent the remaining code from running.
  
//   Serial.println();
//   Serial.println("BNO08x Read Example");

//   Wire.begin(43,44);

//   //if (myIMU.begin() == false) {  // Setup without INT/RST control (Not Recommended)
//   if (myIMU.begin(BNO08X_ADDR, Wire,-1, -1) == false) {
//     Serial.println("BNO08x not detected at default I2C address. Check your jumpers and the hookup guide. Freezing...");
//     while (1)
//       ;
//   }
//   Serial.println("BNO08x found!");

//   // Wire.setClock(400000); //Increase I2C data rate to 400kHz

//   setReports();

//   Serial.println("Reading events");
//   delay(100);
// }

// // Here is where you define the sensor outputs you want to receive
// void setReports(void) {
//   Serial.println("Setting desired reports");
//   if (myIMU.enableGeomagneticRotationVector() == true) {
//     Serial.println(F("Geomagnetic Rotation vector enabled"));
//     Serial.println(F("Output in form roll, pitch, yaw"));
//   } else {
//     Serial.println("Could not enable geomagnetic rotation vector");
//   }
// }

// void loop() {
//   delay(10);

//   if (myIMU.wasReset()) {
//     Serial.print("sensor was reset ");
//     setReports();
//   }

//   // Has a new event come in on the Sensor Hub Bus?
//   if (myIMU.getSensorEvent() == true) {

//     // is it the correct sensor data we want?
//     if (myIMU.getSensorEventID() == SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR) {

//     float roll = (myIMU.getRoll()) * 180.0 / PI; // Convert roll to degrees
//     float pitch = (myIMU.getPitch()) * 180.0 / PI; // Convert pitch to degrees
//     float yaw = (myIMU.getYaw()) * 180.0 / PI; // Convert yaw / heading to degrees

//     Serial.print(roll, 1);
//     Serial.print(F(","));
//     Serial.print(pitch, 1);
//     Serial.print(F(","));
//     Serial.print(yaw, 1);



//     Serial.println();
//     }
//   }
// }