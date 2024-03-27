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
#endif

auto imuTask = Task("IMU Polling", 5000, 2, +[](const uint16_t freq /*, const uint8_t count */){
  const TickType_t delayTime = pdMS_TO_TICKS(1000 / freq);
  Orientation cur;
  while (true) {
    cur = BNO086::poll();
    Serial.printf("Roll: % 5.2f,\tPitch: % 5.2f,\tYaw: % 5.2f\n", cur.roll, cur.pitch, cur.yaw);
    vTaskDelay(delayTime);
  }
  // for (uint8_t i = count; i > 0; --i) {
  //   Serial.printf("I will run %i times\n", i);
  //   vTaskDelay(delayTime);
  // }
});

void setup() {
  Serial.begin(115200);
  
  while(!Serial) delay(10); // Wait for Serial to become available.
  // Necessary for boards with native USB (like the SAMD51 Thing+).
  // For a final version of a project that does not need serial debug (or a USB cable plugged in),
  // Comment out this while loop, or it will prevent the remaining code from running.
  

  delay(3000);

#ifdef PRO_FEATURES
  display.init();

  display.setTextColor(color_rgb(255, 255, 255));
  display.setTextSize(2);
#endif

  msg_assert(BNO086::init(), "Could not initialize IMU");
  imuTask(60);
  redBox.init();
  
}



void loop() {
  delay(10);
  
  redBox.printData(redBox.getData());
    

  // Draw code goes between `display.clear()` and `display.refresh()`
  display.setCursor(10, 80);
  display.print("Hello, Mouseless World!");
  display.setCursor(10, 110);
  display.printf("Frame %i", ++t);

  display.refresh();
#endif
  // if (!imuTask.isRunning) imuTask(2, rand()&7);
  Orientation cur = BNO086::poll();
  Serial.printf("Roll: % 5.2f,\tPitch: % 5.2f,\tYaw: % 5.2f\n", cur.roll, cur.pitch, cur.yaw);
  vTaskDelay(10);
}
