#include <Arduino.h>

#include "sensor.h"

// #define PRO_FEATURES Only define this for MMPro (handled automatically by platformio config when building project)
// wrap statements in #ifdef DEBUG #endif to enable them only in debug builds
#include "sensor.h"
#include "task.h"
#include "debug.h"

#ifdef PRO_FEATURES
#include "display.h"

TFT_Parallel display(320, 170);

auto displayTask = Task("Display", 5000, 2, +[](){
  static uint32_t t = 0;
  while (1) {
    display.clear();

    // Draw code goes between `display.clear()` and `display.refresh()`
    display.setCursor(10, 80);
    display.print("Hello, Mouseless World!");
    display.setCursor(10, 110);
    display.printf("Frame %i", ++t);

    display.refresh();
  }
});

#endif

auto imuTask = Task("IMU Polling", 5000, 1, +[](const uint16_t freq){
  const TickType_t delayTime = pdMS_TO_TICKS(1000 / freq);
  Orientation cur;
  while (true) {
    cur = BNO086::poll();
    Serial.printf("Roll: % 5.2f,\tPitch: % 5.2f,\tYaw: % 5.2f\n", cur.roll, cur.pitch, cur.yaw);
    vTaskDelay(delayTime);
  }
});

void setup() {
  Serial.begin(115200);
  
#ifdef DEBUG
  while(!Serial) delay(10); // Wait for Serial to become available.
  // Necessary for boards with native USB (like the SAMD51 Thing+).
  // For a final version of a project that does not need serial debug (or a USB cable plugged in),
  // Comment out this while loop, or it will prevent the remaining code from running.
#endif

  delay(3000);

#ifdef PRO_FEATURES
  display.init();

  display.setTextColor(color_rgb(255, 255, 255));
  display.setTextSize(2);

  displayTask();
#endif

  msg_assert(BNO086::init(), "Could not initialize BNO086");
  imuTask(100);
}

void loop() {

}
