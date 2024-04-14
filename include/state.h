#pragma once

#include <Arduino.h>
#include <forward_list>
#include <unordered_map>
#include <vector>

namespace Shell {

typedef void (*Command)(std::vector<const char*>&);

void serialCB();

void registerCmd(const char* name, Command cmd);

const std::unordered_map<std::string, Command>& registry();

};  // namespace Shell

class Status {
    std::string name;
    std::forward_list<const char*> errlog;
    decltype(errlog)::iterator logTail;
    bool hasError;
public:
    Status(std::string name);
    operator bool() const;
    void logError(const char* errmsg);
    const std::string& getName() const;
    bool check(bool condition, const char* errmsg);

    static Status* find(const char* searchName);
    const std::forward_list<const char*>& getLog() const;
};

class TaskLog : public Print {
    static std::unordered_map<TaskHandle_t, bool> taskLogRegistry;
public:
    TaskLog();
    ~TaskLog();
    bool isEnabled() const;
    size_t write(const uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    void call(void (*if_monitoring)(void));

    static bool enable(TaskHandle_t task);
    static void disable(TaskHandle_t task);
    static bool isEnabled(TaskHandle_t task);
};