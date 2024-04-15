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

class LogFile : public Print {
    std::string pLog;
public:
    size_t write(const uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    
    const std::string& get();
};

LogFile& TaskLog();
LogFile& TaskLog(TaskHandle_t task);

class TaskPrint : public Print {
    static std::unordered_map<TaskHandle_t, bool> taskMonitorRegistry;
public:
    TaskPrint();
    ~TaskPrint();
    bool isEnabled() const;
    size_t write(const uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    void call(void (*if_monitoring)(void));

    static bool enable(TaskHandle_t task);
    static void disable(TaskHandle_t task);
    static bool isEnabled(TaskHandle_t task);
};