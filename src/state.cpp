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



static std::forward_list<Status*> statusRegistry;

Status::Status(std::string name)
    : name(name)
    , errlog{}
    , logTail(errlog.before_begin())
    , hasError(false)
{
    statusRegistry.push_front(this);
}

Status::operator bool() const {
    return !hasError;
}

void Status::logError(const char* errmsg) {
    hasError = true;
    errlog.insert_after(logTail, errmsg);
    ++logTail;
}

const std::string& Status::getName() const {
    return name;
}

bool Status::check(bool condition, const char* errmsg) {
    if (!condition)
        logError(errmsg);
    return condition;
}

Status* Status::find(const char* searchName) {
    for (Status* state : statusRegistry)
        if (state->getName() == searchName)
            return state;
    return nullptr;
}

const std::forward_list<const char*>& Status::getLog() const {
    return errlog;
}



std::unordered_map<TaskHandle_t, bool> TaskLog::taskLogRegistry;

TaskLog::TaskLog()
{
    auto task = taskLogRegistry.find(xTaskGetCurrentTaskHandle());
    if (task == taskLogRegistry.end()) {
        taskLogRegistry.insert(std::pair<TaskHandle_t, bool>(xTaskGetCurrentTaskHandle(), false));
    }
    else if (task->second) {
        USBSerial.print("\e[2K\e[1G");
    }
}

TaskLog::~TaskLog() {
    auto task = taskLogRegistry.find(xTaskGetCurrentTaskHandle());
    if (task == taskLogRegistry.end()) {
        taskLogRegistry.insert(std::pair<TaskHandle_t, bool>(xTaskGetCurrentTaskHandle(), false));
    }
    else if (task->second) {
        USBSerial.print(serialCmd);
    }
}

bool TaskLog::isEnabled() const {
    auto taskLog = taskLogRegistry.find(xTaskGetCurrentTaskHandle());
    if (taskLog == taskLogRegistry.end())
        return false;
    else
        return taskLog->second;
}

size_t TaskLog::write(const uint8_t c) {
    if (taskLogRegistry.find(xTaskGetCurrentTaskHandle())->second)
        return USBSerial.write(c);
    return 0;
}

size_t TaskLog::write(const uint8_t *buffer, size_t size) {
    if (taskLogRegistry.find(xTaskGetCurrentTaskHandle())->second == false)
        return 0;
    size_t n = 0;
    while (size--) {
        if (write(*buffer++)) n++;
        else break;
    }
    return n;
}

void TaskLog::call(void (*if_monitoring)(void)) {
    if (taskLogRegistry.find(xTaskGetCurrentTaskHandle())->second == false)
        return;
    if_monitoring();
}

bool TaskLog::enable(TaskHandle_t task) {
    auto taskLog = taskLogRegistry.find(task);
    if (taskLog == taskLogRegistry.end())
        return false;
    else
        taskLog->second = true;
    return true;
}

void TaskLog::disable(TaskHandle_t task) {
    auto taskLog = taskLogRegistry.find(task);
    if (taskLog == taskLogRegistry.end())
        return;
    else
        taskLog->second = false;
}

bool TaskLog::isEnabled(TaskHandle_t task) {
    auto taskLog = taskLogRegistry.find(task);
    if (taskLog == taskLogRegistry.end())
        return false;
    else
        return taskLog->second;
}