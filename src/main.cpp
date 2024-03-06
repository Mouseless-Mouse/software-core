#include <Arduino.h>

#include "display.h"

TFT_Parallel display(320, 170);

void setup() {
  // put your setup code here, to run once:
  setCpuFrequencyMhz(240);
  Serial.begin(115200);

  delay(3000);

  display.init();

  display.setTextColor(color_rgb(255, 255, 255));
  display.setTextSize(2);
}

uint32_t startTime = 0;
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

  display.refresh();
}