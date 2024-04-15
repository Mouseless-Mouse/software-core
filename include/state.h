#pragma once

#include <Arduino.h>
#include <forward_list>
#include <unordered_map>
#include <vector>

namespace Shell {

typedef void (*Command)(std::vector<const char*>&);

void init();

void serialCB();

void registerCmd(const char* name, Command cmd);

const std::unordered_map<std::string, Command>& registry();

};  // namespace Shell

class TaskLog : public Print {
    static std::unordered_map<TaskHandle_t, std::string> taskLogRegistry;
    TaskHandle_t task;
public:
    TaskLog();
    TaskLog(TaskHandle_t task);

    size_t write(const uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    
    const std::string& get();
};

template <class T, typename = typename std::enable_if<std::is_base_of<Print, T>::value, void>::type>
class Error : public T {
public:
    Error() : T() { T::print("\e[31mError: "); }
    ~Error() { T::print("\e[0m"); }
};

template <class T, typename = typename std::enable_if<std::is_base_of<Print, T>::value, void>::type>
class Warn : public T {
public:
    Warn() : T() { T::print("\e[33mWarning: "); }
    ~Warn() { T::print("\e[0m"); }
};

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