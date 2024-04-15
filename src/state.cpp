#include "state.h"
#include "usb_classes.h"
#include "taskwrapper.h"

#include <stack>

#define CMD_BUF_SIZE 1024

static char serialCmd[CMD_BUF_SIZE] = {0};
static size_t cmdIdx = 0;
static std::stack<bool (*)(char)> serialParserStack;
static std::unordered_map<std::string, Shell::Command> cmdRegistry;

void parseCmd(char *cmd);
bool baseSerialParser(char c);

bool escapeSeqParser(char c) {
    static char second = '\0';
    static std::string seq;
    if (!second) {
        second = c;
        seq += c;
    }
    else
        switch (second) {
            case '[': {
                seq += c;
                if ('\x40' <= c && c > '\x00') {
                    if (seq == "[D" && cmdIdx > 0) {
                        --cmdIdx;
                        USBSerial.print("\e[D");
                    }
                    else if (seq == "[C" && serialCmd[cmdIdx] != '\0') {
                        ++cmdIdx;
                        USBSerial.print("\e[C");
                    }
                    second = '\0';
                    seq.clear();
                    return false;
                }
            } break;
            default: {
                second = '\0';
                seq.clear();
                return false;
            } break;
        }
    return true;
}

bool baseSerialParser(char c) {
    if (c != '\r' && c != '\n' && c != '\b' && c != '\x7f' && c != '\e') {
        size_t cursorPos = cmdIdx;
        USBSerial.write(c);
        while (serialCmd[cmdIdx])
            ++cmdIdx;
        do {
            serialCmd[cmdIdx+1] = serialCmd[cmdIdx];
        } while(cmdIdx-- > cursorPos);
        cmdIdx = cursorPos;
        serialCmd[cmdIdx++] = c;
        USBSerial.printf("\e7%s\e8", &(serialCmd[cmdIdx]));
    }
    else if ((c == '\b' || c == '\x7f') && cmdIdx > 0) {
        size_t cursorPos = --cmdIdx;
        USBSerial.write(c);
        do {
            serialCmd[cmdIdx] = serialCmd[cmdIdx+1];
        } while(serialCmd[++cmdIdx]);
        cmdIdx = cursorPos;
        USBSerial.printf(" \b\e7%s\e8", &(serialCmd[cmdIdx]));
    }
    else if (c == '\e') {
        serialParserStack.push(escapeSeqParser);
    }
    else if (c == '\n') {
        USBSerial.write(c);
        if (serialCmd[0] != '\0') {
            parseCmd(serialCmd);
            while (serialParserStack.size() != 1)
                serialParserStack.pop();
            serialCmd[0] = '\0';
            cmdIdx = 0;
        }
    }
    return true;
}

void Shell::init() {
    if (serialParserStack.empty())
        serialParserStack.push(baseSerialParser);
}

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
    while (USBSerial.available()) {
        char c = USBSerial.read();
        if (!serialParserStack.top()(c))
            serialParserStack.pop();
    }
}



std::unordered_map<TaskHandle_t, std::string> TaskLog::taskLogRegistry;

size_t TaskLog::write(uint8_t c) {
    taskLogRegistry[task] += c;
    return 1;
}

size_t TaskLog::write(const uint8_t *buffer, size_t size) {
    taskLogRegistry[task].append((const char*)buffer, size);
    return size;
}

const std::string& TaskLog::get() {
    return taskLogRegistry[task];
}

TaskLog::TaskLog() : task(xTaskGetCurrentTaskHandle())
{}

TaskLog::TaskLog(TaskHandle_t task) : task(task)
{}



std::unordered_map<TaskHandle_t, bool> TaskPrint::taskMonitorRegistry;

TaskPrint::TaskPrint()
{
    auto task = taskMonitorRegistry.find(xTaskGetCurrentTaskHandle());
    if (task == taskMonitorRegistry.end()) {
        taskMonitorRegistry.insert(std::pair<TaskHandle_t, bool>(xTaskGetCurrentTaskHandle(), false));
    }
    else if (task->second) {
        USBSerial.print("\e7\e[2K\e[1G");
    }
}

TaskPrint::~TaskPrint() {
    auto task = taskMonitorRegistry.find(xTaskGetCurrentTaskHandle());
    if (task == taskMonitorRegistry.end()) {
        taskMonitorRegistry.insert(std::pair<TaskHandle_t, bool>(xTaskGetCurrentTaskHandle(), false));
    }
    else if (task->second) {
        USBSerial.printf("%s\e8", serialCmd);
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