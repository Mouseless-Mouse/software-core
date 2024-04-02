#include <Arduino.h>

// #define PRO_FEATURES Only define this for MMPro (handled automatically by platformio config when building project)
// wrap statements in #ifdef DEBUG #endif to enable them only in debug builds

#include "display.h"
//#include "Serial.h"

TFT_Parallel display(320, 170);

void setup() {
  // put your setup code here, to run once:
  setCpuFrequencyMhz(240);
  Serial.begin(115200);

  delay(3000);

  display.init();

  display.setTextColor(color_rgb(255, 255, 255));
  display.setTextSize(2);
  //analogWrite(15, 100);
}

long tt = 0;
byte level = 255;
void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t t = 0;
  while (!display.done_refreshing());
  display.clear();

  // Draw code goes between `display.clear()` and `display.refresh()`
  display.setCursor(10, 80);
  display.print("Hello, Mouseless World!");
  display.setCursor(10, 110);
  display.printf("Frame %i", ++t);
  display.setCursor(10, 140);
  //digitalWrite(14, HIGH);
  //delay(10);

  float measurement = (float) analogRead(4);
  float battery_voltage = (measurement / 4095.0) * 7.26;
  display.printf("%f %f %d", measurement, battery_voltage, getBatteryPercentage());
  //Serial.println(bat);
  //digitalWrite(14, LOW);
  

  display.refresh();
  //putting on a test load
  if(millis() - tt >= 5000) {
    tt = millis();
    if(level != 255) {
      level = 255;
      analogWrite(38, level);
    } else {
      level = 50;
      analogWrite(38, level);
    }
  }
}
const uint8_t max_drop_cnt = 2;
uint8_t getBatteryPercentage() {
  float battery_voltage = (analogRead(4) / 4095.0) * 7.26;
  static uint8_t filtered_level = 101;
  static uint8_t drop_cnt = 0;
  uint8_t level = 100; 
  //https://blog.ampow.com/lipo-voltage-chart/
  if(battery_voltage >= 4.2) {
    level = 100;
  } else if(battery_voltage >= 4.11) {
    level = 90;
  } else if(battery_voltage >= 4.02) {
    level = 80;
  } else if(battery_voltage >= 3.95) {
    level = 70;
  } else if(battery_voltage >= 3.87) {
    level = 60;
  } else if(battery_voltage >= 3.84) {
    level = 50;
  } else if(battery_voltage >= 3.80) {
    level = 40;
  } else if(battery_voltage >= 3.77) {
    level = 30;
  } else if(battery_voltage >= 3.73) {
    level = 20;
  } else if(battery_voltage >= 3.69) {
    level = 10;
  } else if(battery_voltage >= 3.61) {
    level = 5;
  } else {
    //critical level
    level = 0;
  }
  if(filtered_level == 101) {
    filtered_level = level;
  }
  if(level < filtered_level) {
    ++drop_cnt;
    level = filtered_level;
  }
  if(drop_cnt >= max_drop_cnt) {
    //drop down
    drop_cnt = 0;
    filtered_level = level;
  }
  if(filtered_level < level) {
    level = filtered_level;
  }
  return level;
}
