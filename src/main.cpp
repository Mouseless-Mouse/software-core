#include <Arduino.h>
#include <BleMouse.h>
#include <FFat.h>
#include "esp_core_dump.h"

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

duk_context *duk;
static duk_ret_t native_print(duk_context *ctx) {
  USBSerial.println(duk_to_string(ctx, 0));
  return 0;
}

#ifdef PRO_FEATURES
#include "3ml_renderer.h"
#include "display.h"

TFT_Parallel display(320, 170);

threeml::Renderer renderer(&display);

auto drawTask = Task("Draw Task", 50000, 1, []() {
    uint32_t t = 0;

    display.init();
    display.setTextWrap(false);

    display.setTextColor(color_rgb(255, 255, 255));
    display.setTextSize(2);

    TickType_t wakeTime = xTaskGetTickCount();
    renderer.load_file("/index.3ml");

    size_t runningBehind = 0;
    while (true) {
        renderer.render();
        if (xTaskDelayUntil(&wakeTime, pdMS_TO_TICKS(34)) == pdFALSE) {
            ++runningBehind;
            if (runningBehind >= 10) {
                Warn<TaskLog>().println("Draw task is running behind!");
                runningBehind = 0;
            }
        }
    }
});

#else

// Basic version `draw` controls LED_BUILTIN
auto drawTask = Task("Draw Task", 5000, 1, []() {
    static uint32_t t = 0;

    ledcSetup(BASIC_LEDC_CHANNEL, 1000, 8);
    ledcAttachPin(LED_BUILTIN, BASIC_LEDC_CHANNEL);

    TickType_t wakeTime = xTaskGetTickCount();

    while (1) {
        ledcWrite(BASIC_LEDC_CHANNEL, 128 + 127 * sin(t / 120.f * 2 * PI));
        ++t;
        vTaskDelayUntil(&wakeTime, pdMS_TO_TICKS(17));
    }
});

#endif

auto imuTask = Task("IMU Polling", 5000, 1, [](const uint16_t freq) {
    const TickType_t delayTime = pdMS_TO_TICKS(1000 / freq);
    Orientation cur;
    if (!BNO086::init(false)) {
        Error<TaskLog>().println("Failed to initialize BNO086");
        while (1) vTaskDelay(portMAX_DELAY);    // Keep the task alive so its log can be fetched
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    uint32_t t = 0;
    mouse.begin();
    mouseInitialized = true;
    while (1) {
        cur = BNO086::poll();
        mouse.move(
            clamp(-50, static_cast<int>(pow(cur.pitch, 3) / 60), 50),
            clamp(-50, static_cast<int>(pow(cur.roll, 3) / 60), 50)
        );
        if (++t % 32 == 0) {
            TaskPrint().printf("Roll: % 7.2f, Pitch: % 7.2f, Yaw: % 7.2f\n", cur.roll, cur.pitch, cur.yaw);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
});

auto touchTask = Task("Touch Reporting", 4000, 1, []() {
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

#ifdef PRO_FEATURES
auto displayDimTest = Task("Display Dimmer", 3000, 1, []() {
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
#endif

namespace ShellCommands {

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

void getTaskLog(const std::vector<const char*>& args) {
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
        if (strcmp("timer", target) != 0) {
            USBSerial.printf("Status '%s' not found\n", target);
            return;
        }
        else {
            status = xTimerGetTimerDaemonTaskHandle();
        }
    }
    USBSerial.println("Log entries:");
    USBSerial.print(TaskLog(status).get().c_str()); // Log should end with newline
}

void toggleMonitor(const std::vector<const char*>& args) {
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

void systemStatus(const std::vector<const char *> &args) {
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

void treeCmd(const std::vector<const char *> &args) {
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

void mouseSay(const std::vector<const char*>& args){
    static const char mousey[] = "         %s\n         /\n(\\   /) /  _\n (0 0)____  \\\n \"\\ /\"    \\ /\n  |' ___   /\n   \\/   \\_/\n";

    std::string result;
    for (const char *arg : args) {
        result += arg;
        result += " ";
    }
    if (!result.empty())
        result.pop_back();
    USBSerial.printf(mousey, result.c_str());
}

void helpMePlz(const std::vector<const char *> &args) {
    if (!args.empty()) {
        USBSerial.println("Help for specific commands is NYI.");
    }
    USBSerial.println("Registered commands:");
    for (const std::pair<std::string, Shell::Command> &p : Shell::registry())
        USBSerial.printf(" - %s\n", p.first.c_str());
}

void getCrashReason(const std::vector<const char*>& args) {
    if (!args.empty()) {
        USBSerial.println("Expected no arguments");
    }
    size_t coreDumpLoc;
    size_t coreDumpSz;
    if (esp_core_dump_image_get(&coreDumpLoc, &coreDumpSz) != ESP_OK) {
        USBSerial.println("Could not get core dump image");
        return;
    }
    esp_core_dump_summary_t summary;
    if (esp_core_dump_get_summary(&summary) != ESP_OK) {
        USBSerial.println("Could not retrieve core dump summary");
        return;
    }
    esp_core_dump_bt_info_t& backtrace = summary.exc_bt_info;
    USBSerial.printf("Crash caused by task '%s' (Error Code %i):\n", summary.exc_task, summary.ex_info.exc_cause);
    if (backtrace.corrupted)
        USBSerial.println("Backtrace is corrupted");
    for (size_t i = 0; i < backtrace.depth; ++i)
        USBSerial.printf("%#0.8x ", backtrace.bt[i]);
    USBSerial.println();
}

}   // namespace ShellCommands

auto deferredPrinter = Task("Shell Greeter", 3000, 1, [](){
    while (!USBSerial) vTaskDelay(pdMS_TO_TICKS(100));
    USBSerial.print("\e[2J\e[1;1H"
        "|\\  /|           _  _  |  _   _  _    |\\  /|           _  _\n"
        "| \\/ | /\"\\ |  | /_ /_| | /_| /_ /_    | \\/ | /\"\\ |  | /_ /_|\n"
        "|    | \\_/ \\_/| _/ \\_  | \\_  _/ _/    |    | \\_/ \\_/| _/ \\_\n\n"
        "Welcome to the Mouseless Debug Terminal! Type 'help' for a list of available commands.\n\n"
    );
    if (esp_reset_reason() == ESP_RST_PANIC) {
        ShellCommands::getCrashReason(std::vector<const char*>());
    }
    USBSerial.print("\e[92mdev@mouseless\e[0m:\e[36m/\e[0m$ ");
});

void setup() {
    /*
        Unit testing block - Please place unit tests here until someone comes up with a better place for them
    */

    // UnitTest::add("js", [](){
    //     duk = duk_create_heap_default();
    //     duk_push_c_function(duk, native_print, 1);
    //     duk_put_global_string(duk, "print");
    //     duk_eval_string(duk,
    //         "var fib = function(n) {"
    //             "return n < 2 ? n : fib(n-2) + fib(n-1)"
    //         "};"
    //         "print(fib(6));"
    //     );
    //     USBSerial.println((int)duk_get_int(duk, -1));
    //     duk_destroy_heap(duk);
    // });

#ifdef PRO_FEATURES
    UnitTest::add("backlight", []() {
        if (displayDimTest.isRunning)
            displayDimTest.stop();
        else
            displayDimTest();
    });
#endif

    /*
        End of unit testing block
    */

    Shell::init();

    Shell::onConnect([](){
        if (!deferredPrinter.isRunning)
            deferredPrinter();
    });

    Shell::registerCmd("log", ShellCommands::getTaskLog);
    Shell::registerCmd("monitor", ShellCommands::toggleMonitor);
    Shell::registerCmd("memory", ShellCommands::systemStatus);
    Shell::registerCmd("test", UnitTest::run);
    Shell::registerCmd("mousesay", ShellCommands::mouseSay);
    Shell::registerCmd("crashdump", ShellCommands::getCrashReason);
    Shell::registerCmd("help", ShellCommands::helpMePlz);

    initUSB();
    initSerial();
    initMSC();

    if (FFat.begin(false)) {
        Shell::registerCmd("tree", ShellCommands::treeCmd);
    }
    else {
        USBSerial.println("Failed to initialize filesystem");
    }

#ifdef PRO_FEATURES
    renderer.init();
#endif

    drawTask();
    touchTask();
    imuTask(90);
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
