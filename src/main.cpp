#include <Arduino.h>
#include <BleMouse.h>
#include <FFat.h>

// #define PRO_FEATURES Only define this for MMPro (handled automatically by
// platformio config when building project) wrap statements in #ifdef DEBUG
// #endif to enable them only in debug builds
#include "debug.h"
#include "sensor.h"
#include "state.h"
#include "taskwrapper.h"
#include "touch.h"
#include "usb_classes.h"
#include "button.h"
#include "unit_testing.h"

extern "C" {
#include "duktape.h"
}

#define BASIC_LEDC_CHANNEL 2

BleMouse mouse("Mouseless Mouse " __TIME__, "The Mouseless Gang", 69U);

Global<bool> mouseInitialized(false);

Button downButton(14);

duk_context *duk;
static duk_ret_t native_print(duk_context *ctx) {
  USBSerial.println(duk_to_string(ctx, 0));
  return 0;
}

#ifdef PRO_FEATURES
#include "3ml_renderer.h"
#include "display.h"

TFT_Parallel display(320, 170);

threeml::Renderer renderer(&display, nullptr);

auto drawTask = Task("Draw Task", 5000, 1, +[]() {
    uint32_t t = 0;

    display.init();
    display.setTextWrap(false);

    display.setTextColor(color_rgb(255, 255, 255));
    display.setTextSize(2);

    TickType_t wakeTime = xTaskGetTickCount();

    size_t runningBehind = 0;
    while (1) {
        // TickType_t start = xTaskGetTickCount();

        renderer.render();
        // while (!display.done_refreshing());
        // display.clear();

        // // Draw code goes between `display.clear()` and `display.refresh()`
        // display.setCursor(10, 10);
        // display.print("Hello, Mouseless World!");
        // display.setCursor(10, 40);
        // display.printf("Frame %i", ++t);
        // display.setCursor(10, 70);
        // display.printf("USB %s", usbMounted ? "Connected" : "Disconnected");

        // display.refresh();

        // TickType_t finish = xTaskGetTickCount();
        // display.setCursor(10, 100);
        // display.print(finish - start);

        if (xTaskDelayUntil(&wakeTime, pdMS_TO_TICKS(17)) == pdFALSE) {
            ++runningBehind;
            if (runningBehind == 10)
                Warn<TaskLog>().println("Draw task is running behind!");
        }
    }
});

#else

// Basic version `draw` controls LED_BUILTIN
auto drawTask = Task("Draw Task", 5000, 1, +[]() {
    static uint32_t t = 0;

    ledcSetup(BASIC_LEDC_CHANNEL, 1000, 8);
    ledcAttachPin(LED_BUILTIN, BASIC_LEDC_CHANNEL);

    while (1) {
        ledcWrite(BASIC_LEDC_CHANNEL, 128 + 127 * sin(t / 120.f * 2 * PI));
        ++t;
    }
});

#endif

auto imuTask = Task("IMU Polling", 5000, 1, +[](const uint16_t freq) {
    const TickType_t delayTime = pdMS_TO_TICKS(1000 / freq);
    Orientation cur;
    if (!BNO086::init()) {
        Error<TaskLog>().println("Failed to initialize BNO086");
        return;
    }
    uint32_t t = 0;
    mouse.begin();
    mouseInitialized = true;
    while (1) {
        cur = BNO086::poll();
        mouse.move(pow(cur.pitch, 3) / 60, pow(cur.roll, 3) / 60);
        if (t % 32 == 0) {
            TaskPrint().printf("Roll: % 7.2f, Pitch: % 7.2f, Yaw: % 7.2f\n", cur.roll, cur.pitch, cur.yaw);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
});

auto touchTask = Task("Touch Reporting", 4000, 1, +[]() {
    TouchPads::init<TOUCH_PAD_NUM1, TOUCH_PAD_NUM2>(60000);
    while (1) {
        static uint32_t result1;
        touch_pad_read_raw_data(TOUCH_PAD_NUM1, &result1);
        TaskPrint().printf("Touch State: %i %i\n",
            TouchPads::status[TOUCH_PAD_NUM1],
            TouchPads::status[TOUCH_PAD_NUM2]
        );
        if (mouseInitialized && TouchPads::status[TOUCH_PAD_NUM1]) {
            mouse.press(MOUSE_LEFT);
        }
        else if (mouseInitialized) {
            mouse.release(MOUSE_LEFT);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
});

auto displayDimTest = Task("Display Dimmer", 3000, 1, +[]() {
    static uint8_t brightness = 255;
    static int8_t dir = -1;

    while (1) {
        brightness += dir;
        display.set_backlight(brightness);
        if (brightness == 255 || brightness == 32)
            dir *= -1;
        
        vTaskDelay(pdMS_TO_TICKS(2));
    }
});

auto deferredPrinter = Task("Shell Greeter", 3000, 1, +[](){
    while (!USBSerial) vTaskDelay(pdMS_TO_TICKS(100));
    USBSerial.print("\e[2J\e[1;1H"
        "|\\  /|           _  _  |  _   _  _    |\\  /|           _  _\n"
        "| \\/ | /\"\\ |  | /_ /_| | /_| /_ /_    | \\/ | /\"\\ |  | /_ /_|\n"
        "|    | \\_/ \\_/| _/ \\_  | \\_  _/ _/    |    | \\_/ \\_/| _/ \\_\n\n"
        "Welcome to the Mouseless Debug Terminal! Type 'help' for a list of available commands.\n\n"
        "\e[92mdev@mouseless\e[0m:\e[36m/\e[0m$ "
    );
});

void treeList(File &dir, int level = 0) {
    if (!dir || !dir.isDirectory()) {
        USBSerial.println("treeList called on non-directory");
        return;
    }
    File file = dir.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            for (int i = 0; i < level; ++i)
                USBSerial.print("  ");
            USBSerial.printf("%s:\n", file.name());
            treeList(file, level + 1);
        }
        else {
            for (int i = 0; i < level; ++i)
                USBSerial.print("  ");
            USBSerial.println(file.name());
        }
        file = dir.openNextFile();
    }
}

void getTaskLog(std::vector<const char*>& args) {
    if (args.size() != 1) {
        USBSerial.println("Expected 1 argument");
        return;
    }
    const char *target = args.front();
    if (strnlen(target, configMAX_TASK_NAME_LEN) > configMAX_TASK_NAME_LEN - 1) {
        USBSerial.printf("Error: Maximum task name length is %i characters\n", configMAX_TASK_NAME_LEN - 1);
        return;
    }
    TaskHandle_t status = xTaskGetHandle(target);
    if (!status) {
        USBSerial.printf("Status '%s' not found\n", target);
        return;
    }
    USBSerial.println("Log entries:");
    USBSerial.print(TaskLog(status).get().c_str()); // Log should end with newline
}

void toggleMonitor(std::vector<const char*>& args) {
    if (args.size() != 1) {
        USBSerial.println("Expected 1 argument");
        return;
    }
    const char *target = args.front();
    if (strnlen(target, configMAX_TASK_NAME_LEN) > configMAX_TASK_NAME_LEN - 1) {
        USBSerial.printf("Error: Maximum task name length is %i characters\n", configMAX_TASK_NAME_LEN - 1);
        return;
    }
    TaskHandle_t monitorTask = xTaskGetHandle(target);
    if (!monitorTask) {
        if (strcmp("timer", target) != 0) {
            USBSerial.printf("Monitor '%s' not found\n", target);
            return;
        }
        else {
            monitorTask = xTimerGetTimerDaemonTaskHandle();
        }
    }
    if (TaskPrint::isEnabled(monitorTask)) {
        TaskPrint::disable(monitorTask);
        USBSerial.printf("Stopped monitoring '%s'\n", target);
    }
    else {
        USBSerial.printf("Monitoring '%s'\n", target);
        TaskPrint::enable(monitorTask);
    }
}

void systemStatus(std::vector<const char *> &args) {
    if (!args.empty()) {
        USBSerial.println("Expected no arguments");
        return;
    }
    USBSerial.printf("Total free heap: %i\n", xPortGetFreeHeapSize());
    USBSerial.printf("Minimum free heap reached: %i\n", esp_get_minimum_free_heap_size());
    USBSerial.printf("Free internal heap: %i\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    USBSerial.printf("Minium free internal heap reached: %i\n", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    USBSerial.printf("Largest free internal heap block: %i\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    USBSerial.printf("Free SPIRAM heam: %i\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    USBSerial.printf("Minium free SPIRAM heap reached: %i\n", heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
    USBSerial.printf("Largest free SPIRAM heap block: %i\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

void treeCmd(std::vector<const char *> &args) {
    if (args.empty()) {
        File root = FFat.open("/");
        treeList(root);
        return;
    }
    else if (args.size() > 1) {
        USBSerial.println("Too many arguments");
        return;
    }
    File target = FFat.open(args.front());
    if (!target) {
        USBSerial.printf("Could not open directory '%s'\n", args.front());
        return;
    }
    treeList(target);
}

void helpMePlz(std::vector<const char *> &args) {
    if (!args.empty()) {
        USBSerial.println("Help for specific commands is NYI.");
    }
    USBSerial.println("Registered commands:");
    for (const std::pair<std::string, Shell::Command> &p : Shell::registry())
        USBSerial.printf(" - %s\n", p.first.c_str());
}

void setup() {

    auto dom = threeml::clean_dom(
        threeml::parse_string("<head><title>Test</title></head><body><h1>Hello, "
                              "Mouseless World!</h1>And hello to you, too!</body>"));
    renderer.set_dom(dom);

    downButton
    .on(Button::Event::PRESS, [](){
        TaskPrint().println("Boop!");
    })
    .on(Button::Event::RELEASE, [](){
        TaskPrint().println("Un-Boop!");
    })
    .on(Button::Event::CLICK, [](){
        TaskPrint().println("Short Boop");
    })
    .on(Button::Event::HOLD, [](){
        TaskPrint().println("Long Boop");
    });

    downButton.attach();

    /*
        Unit testing block - Please place unit tests here until someone comes up with a better place for them
    */

    UnitTest::add("js", [](){
        duk = duk_create_heap_default();
        duk_push_c_function(duk, native_print, 1);
        duk_put_global_string(duk, "print");
        duk_eval_string(duk,
            "var fib = function(n) {"
                "return n < 2 ? n : fib(n-2) + fib(n-1)"
            "};"
            "print(fib(6));"
        );
        USBSerial.println((int)duk_get_int(duk, -1));
        duk_destroy_heap(duk);
    });

    UnitTest::add("backlight", []() {
        if (displayDimTest.isRunning)
            displayDimTest.stop();
        else
            displayDimTest();
    });

    /*
        End of unit testing block
    */

    Shell::init();

    Shell::onConnect([](){
        if (!deferredPrinter.isRunning)
            deferredPrinter();
    });

    Shell::registerCmd("log", getTaskLog);
    Shell::registerCmd("monitor", toggleMonitor);
    Shell::registerCmd("memory", systemStatus);
    Shell::registerCmd("test", UnitTest::run);
    Shell::registerCmd("help", helpMePlz);

    initUSB();
    initSerial();
    initMSC();

    if (FFat.begin(false)) {
        Shell::registerCmd("tree", treeCmd);
    }
    else {
        USBSerial.println("Failed to initialize filesystem");
    }

    drawTask();
    touchTask();
    imuTask(10);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
