#include <Arduino.h>

#include "sensor.h"

// #define PRO_FEATURES Only define this for MMPro (handled automatically by platformio config when building project)
// wrap statements in #ifdef DEBUG #endif to enable them only in debug builds
#include "sensor.h"
#include "taskwrapper.h"
#include "debug.h"
#include "usb_classes.h"

extern "C" {
#include "duktape.h"
}

#define BASIC_LEDC_CHANNEL 2

StaticTimer_t drawTimerBuf;
TimerHandle_t drawCbTimer;

Global<bool> usbConnState(false);
Global<bool> serialState(false);

duk_context *duk;
static duk_ret_t native_print(duk_context *ctx) {
    USBSerial.println(duk_to_string(ctx, 0));
    return 0;
}

#ifdef PRO_FEATURES
#include "display.h"

TFT_Parallel display(320, 170);

void draw(TimerHandle_t timer) {
    (void) timer;
    static uint32_t t = 0;

    while (!display.done_refreshing());
    display.clear();

    // Draw code goes between `display.clear()` and `display.refresh()`
    display.setCursor(10, 10);
    display.print("Hello, Mouseless World!");
    display.setCursor(10, 40);
    display.printf("Frame %i", ++t);
    display.setCursor(10, 70);
    display.printf("Serial %s", serialState ? "Connected" : "Disconnected");
    display.setCursor(10, 100);
    display.print(USBSerial.getBitrate());

    display.refresh();
}

#else
// Basic version `draw` controls LED_BUILTIN
void draw(TimerHandle_t timer) {
    (void) timer;
    static uint32_t t = 0;
    ledcWrite(BASIC_LEDC_CHANNEL, 128 + 127 * sin(t / 120.f * 2 * PI));
    ++t;
}

#endif

auto imuTask = Task("IMU Polling", 5000, 1, +[](const uint16_t freq){
    const TickType_t delayTime = pdMS_TO_TICKS(1000 / freq);
    Orientation cur;
    while (true) {
        cur = BNO086::poll();
        USBSerial.printf("Roll: % 7.2f, Pitch: % 7.2f, Yaw: % 7.2f\n", cur.roll, cur.pitch, cur.yaw);
        vTaskDelay(delayTime);
    }
});

auto usbConnStateTask = Task("USB Connection State Monitoring", 4000, 1, +[](){
    while (1) {
        usbConnState = true;    // Find something that can tell whether USB is connected
        vTaskDelay(pdMS_TO_TICKS(100));
    }
});

void setup() {
    Serial.begin(115200);

#ifdef DEBUG
    // while(!Serial) delay(10); // Wait for Serial to become available.
    // Necessary for boards with native USB (like the SAMD51 Thing+).
    // For a final version of a project that does not need serial debug (or a USB cable plugged in),
    // Comment out this while loop, or it will prevent the remaining code from running.
#endif

    delay(3000);

    serialState = initSerial();
    initMSC();
    // usbConnStateTask();

#ifdef PRO_FEATURES
    display.init();

    display.setTextColor(color_rgb(255, 255, 255));
    display.setTextSize(2);
#else
    ledcSetup(BASIC_LEDC_CHANNEL, 1000, 8);
    ledcAttachPin(LED_BUILTIN, BASIC_LEDC_CHANNEL);
#endif

    drawCbTimer = xTimerCreateStatic("Draw Callback Timer", pdMS_TO_TICKS(17), true, NULL, draw, &drawTimerBuf);
    xTimerStart(drawCbTimer, portMAX_DELAY);

    while (!USBSerial);
    duk = duk_create_heap_default();
    duk_push_c_function(duk, native_print, 1);
    duk_put_global_string(duk, "print");
    duk_eval_string(duk, "var fib = function(n){return n < 2 ? n : fib(n-2) + fib(n-1)}; print(fib(6));");
    USBSerial.println((int) duk_get_int(duk, -1));
    duk_destroy_heap(duk);

    if (BNO086::init())
        imuTask(100);
    else {
        while (1) {
            USBSerial.println("Could not initialize BNO086");
            delay(1000);
        }
    }
}

void loop() {
    
}
