#include "state.h"
#include "usb_classes.h"
#include "taskwrapper.h"

#define CMD_BUF_SIZE 1024

static char serialCmd[CMD_BUF_SIZE] = {0};
static std::unordered_map<std::string, Shell::Command> cmdRegistry;

void Shell::registerCmd(const char* name, Command cmd) {
    std::pair<std::string, Command> obj(name, cmd);
    cmdRegistry.insert(obj);
}

const std::unordered_map<std::string, Shell::Command>& Shell::registry() {
    return cmdRegistry;
}

auto eval = Task("Shell Evaluator", 5000, 1, +[](decltype(std::bind(std::declval<Shell::Command>(), std::declval<std::vector<const char*>&>())) boundCmd){
    boundCmd();
});

void parseCmd(char *cmd) {
    char* word = cmd;
    std::string name;
    std::vector<const char*> args;
    bool inQuotes = false;
    size_t i = 0;
    while (cmd[i]) {
        if (!isspace(cmd[i]) || inQuotes) {
            if (cmd[i] == '"') {
                if (!inQuotes)
                    word = &(cmd[i+1]);
                else
                    cmd[i] = '\0';
                inQuotes ^= true;
            }
        }
        else if (word < &(cmd[i])) {
            cmd[i] = '\0';
            if (name.empty())
                name = std::string(word);
            else
                args.emplace_back(word);
            word = &(cmd[i+1]);
        }
        ++i;
    }

    if (word < &(cmd[i])) {
        if (name.empty())
            name = std::string(word);
        else
            args.emplace_back(word);
        word = &(cmd[i+1]);
    }

    // Lookup and execute the command
    auto cmdEntry = cmdRegistry.find(name);
    if (cmdEntry != cmdRegistry.end())
        eval(std::bind(cmdEntry->second, args));
    else
        USBSerial.printf("Unrecognized commmand '%s'\n", name.c_str());
}

void Shell::serialCB() {
    static size_t i = 0;
    while (USBSerial.available()) {
        char c = USBSerial.read();
        USBSerial.write(c);
        if (c != '\r' && c != '\n' && c != '\b' && c != '\x7f') {
            serialCmd[i++] = c;
            serialCmd[i] = '\0';
        }
        else if ((c == '\b' || c == '\x7f') && i > 0) {
            serialCmd[--i] = 0;
            USBSerial.print(" \b");
        }
        else if (c == '\n' && i > 0) {
            parseCmd(serialCmd);
            serialCmd[0] = '\0';
            i = 0;
        }
    }
}



static std::unordered_map<TaskHandle_t, LogFile> taskLogRegistry;

size_t LogFile::write(uint8_t c) {
    pLog += c;
    return 1;
}

size_t LogFile::write(const uint8_t *buffer, size_t size) {
    pLog.append((const char*)buffer, size);
    return size;
}

const std::string& LogFile::get() {
    return pLog;
}

LogFile& TaskLog() {
    TaskHandle_t callingTask = xTaskGetCurrentTaskHandle();
    return taskLogRegistry[callingTask];
}

LogFile& TaskLog(TaskHandle_t task) {
    return taskLogRegistry[task];
}



std::unordered_map<TaskHandle_t, bool> TaskPrint::taskMonitorRegistry;

TaskPrint::TaskPrint()
{
    auto task = taskMonitorRegistry.find(xTaskGetCurrentTaskHandle());
    if (task == taskMonitorRegistry.end()) {
        taskMonitorRegistry.insert(std::pair<TaskHandle_t, bool>(xTaskGetCurrentTaskHandle(), false));
    }
    else if (task->second) {
        USBSerial.print("\e[2K\e[1G");
    }
}

TaskPrint::~TaskPrint() {
    auto task = taskMonitorRegistry.find(xTaskGetCurrentTaskHandle());
    if (task == taskMonitorRegistry.end()) {
        taskMonitorRegistry.insert(std::pair<TaskHandle_t, bool>(xTaskGetCurrentTaskHandle(), false));
    }
    else if (task->second) {
        USBSerial.print(serialCmd);
    }
}

bool TaskPrint::isEnabled() const {
    auto taskLog = taskMonitorRegistry.find(xTaskGetCurrentTaskHandle());
    if (taskLog == taskMonitorRegistry.end())
        return false;
    else
        return taskLog->second;
}

size_t TaskPrint::write(const uint8_t c) {
    if (taskMonitorRegistry.find(xTaskGetCurrentTaskHandle())->second)
        return USBSerial.write(c);
    return 0;
}

size_t TaskPrint::write(const uint8_t *buffer, size_t size) {
    if (taskMonitorRegistry.find(xTaskGetCurrentTaskHandle())->second == false)
        return 0;
    size_t n = 0;
    while (size--) {
        if (write(*buffer++)) n++;
        else break;
    }
    return n;
}

void TaskPrint::call(void (*if_monitoring)(void)) {
    if (taskMonitorRegistry.find(xTaskGetCurrentTaskHandle())->second == false)
        return;
    if_monitoring();
}

bool TaskPrint::enable(TaskHandle_t task) {
    auto taskLog = taskMonitorRegistry.find(task);
    if (taskLog == taskMonitorRegistry.end())
        return false;
    else
        taskLog->second = true;
    return true;
}

void TaskPrint::disable(TaskHandle_t task) {
    auto taskLog = taskMonitorRegistry.find(task);
    if (taskLog == taskMonitorRegistry.end())
        return;
    else
        taskLog->second = false;
}

bool TaskPrint::isEnabled(TaskHandle_t task) {
    auto taskLog = taskMonitorRegistry.find(task);
    if (taskLog == taskMonitorRegistry.end())
        return false;
    else
        return taskLog->second;
}