#include <Arduino.h>

#include "sensor.h"

// #define PRO_FEATURES Only define this for MMPro (handled automatically by platformio config when building project)
// wrap statements in #ifdef DEBUG #endif to enable them only in debug builds
#include "sensor.h"
#include "taskwrapper.h"
#include "debug.h"
#include "usb_classes.h"
#include "touch.h"

extern "C" {
#include "duktape.h"
}

volatile bool usbConnState = false;
volatile bool serialState = false;

duk_context *duk;
static duk_ret_t native_print(duk_context *ctx) {
    USBSerial.println(duk_to_string(ctx, 0));
    return 0;
}

#ifdef PRO_FEATURES
#include "display.h"

TFT_Parallel display(320, 170);

auto displayTask = Task("Display", 5000, 1, +[](){
    static uint32_t t = 0;
    char c = '_';
    std::string msg = "";
    while (1) {
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
        display.setCursor(10, 130);
        display.print(msg.c_str());

        display.refresh();
        vTaskDelay(pdMS_TO_TICKS(1));   // I hate multithreading <3
        while (!display.done_refreshing());
    }
});

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

auto touchReportTask = Task("Touch State Reporting", 4000, 1, +[](){
    TouchPads::init<TOUCH_PAD_NUM1, TOUCH_PAD_NUM2>(80000);
    while (1) {
        uint32_t touchStatus;
        touch_pad_read_raw_data(TOUCH_PAD_NUM1, &touchStatus);
        USBSerial.printf("Touch status: %i %i\n",
            TouchPads::status[TOUCH_PAD_NUM1],
            TouchPads::status[TOUCH_PAD_NUM2]
        );
        vTaskDelay(pdMS_TO_TICKS(100));
    }
});

void setup() {
    Serial.begin(115200);

    delay(3000);

    serialState = initSerial();
    initMSC();

#ifdef DEBUG
    while(!USBSerial) delay(10); // Wait for Serial to become available.
    // Necessary for boards with native USB (like the SAMD51 Thing+).
    // For a final version of a project that does not need serial debug (or a USB cable plugged in),
    // Comment out this while loop, or it will prevent the remaining code from running.
#endif

    // usbConnStateTask();

#ifdef PRO_FEATURES
    display.init();

    display.setTextColor(color_rgb(255, 255, 255));
    display.setTextSize(2);

    displayTask();
#endif

    duk = duk_create_heap_default();
    duk_push_c_function(duk, native_print, 1);
    duk_put_global_string(duk, "print");
    duk_eval_string(duk, "var fib = function(n){return n < 2 ? n : fib(n-2) + fib(n-1)}; print(fib(6));");
    USBSerial.println((int) duk_get_int(duk, -1));
    duk_destroy_heap(duk);

    touchReportTask();
    // if (BNO086::init())
    //     imuTask(100);
    // else {
    //     USBSerial.println("Could not initialize BNO086");
    // }
}

void loop() {
    
}
