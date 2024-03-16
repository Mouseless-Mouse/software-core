#include <Arduino.h>

// #define PRO_FEATURES Only define this for MMPro (handled automatically by platformio config when building project)
// wrap statements in #ifdef DEBUG #endif to enable them only in debug builds

#ifdef PRO_FEATURES
  #include "display.h"

  TFT_Parallel display(320, 170);
#endif

void setup() {
  // put your setup code here, to run once:
  setCpuFrequencyMhz(240);
  Serial.begin(115200);

  delay(3000);

#ifdef PRO_FEATURES
  display.init();

  display.setTextColor(color_rgb(255, 255, 255));
  display.setTextSize(2);
#endif
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t t = 0;
#ifdef PRO_FEATURES
  while (!display.done_refreshing());
  display.clear();

  // Draw code goes between `display.clear()` and `display.refresh()`
  display.setCursor(10, 80);
  display.print("Hello, Mouseless World!");
  display.setCursor(10, 110);
  display.printf("Frame %i", ++t);

  display.refresh();
#endif
}
