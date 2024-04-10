#pragma once

#include <Arduino.h>
#include <utility>
#include "int_seq.h"

typedef void* ParameterPtr;

template <typename... Ts>
struct TaskContainer {
    typedef void (*TaskFn_t)(Ts...);
    TaskHandle_t handle;
    TaskFn_t taskFn;
    const char *name;
    const configSTACK_DEPTH_TYPE stackDepth;
    const UBaseType_t priority;
    std::tuple<Ts...> *params;
    bool isRunning;

    TaskContainer(const char *name, const configSTACK_DEPTH_TYPE stackDepth, const UBaseType_t priority, TaskFn_t taskFn)
        : handle(nullptr)
        , taskFn(taskFn)
        , name(name)
        , stackDepth(stackDepth)
        , priority(priority)
        , params(nullptr)
        , isRunning(false)
    {}
    ~TaskContainer() {
        if (handle)
            vTaskDelete(handle);
        if (params)
            delete params;
    }

    // Use a packed `index_sequence` to unpack the `params` tuple into the arguments of a call to `taskFn`
    template <size_t... Is>
    struct ApplyWrapper {
        static inline void apply(void *instPtr) {
            TaskContainer<Ts...>* inst = reinterpret_cast<TaskContainer<Ts...>*>(instPtr);
            inst->taskFn(std::get<Is>(*inst->params) ...);
            delete inst->params;
            inst->params = nullptr;
            inst->isRunning = false;
            inst->handle = nullptr;
            vTaskDelete(NULL);
        }
    };

    // Pack an `index_sequence` into a template parameter pack of consecutive `size_t`s
    template <size_t... Is>
    static ApplyWrapper<Is...> wrap(std14::index_sequence<Is...>) {
        return ApplyWrapper<Is...>();
    }
    
    bool operator () (Ts... args) {
        if (params) delete params;
        params = new std::tuple<Ts...>{args...};
        return isRunning = pdPASS == xTaskCreatePinnedToCore(
            // Get a function pointer to the instantiation of `ApplyWrapper::apply(void *inst)` that matches and can unpack the `params` tuple
            decltype(wrap(std::declval<std14::index_sequence_for<Ts...>>()))::apply,
            name,
            stackDepth,
            this,
            priority,
            &handle,
            1
        );
    }

    void stop() {
        if (handle) {
            vTaskDelete(handle);
            handle = nullptr;
            isRunning = false;
            delete params;
        }
    }
};

// Factory function to work around the lack of CTAD in C++14
template <typename... Ts>
TaskContainer<Ts...> Task(const char *&&name, const configSTACK_DEPTH_TYPE &&stackDepth, const UBaseType_t &&priority, void (*&&taskFn)(Ts...)) {
    return TaskContainer<Ts...>(std::forward<const char*>(name), std::forward<const configSTACK_DEPTH_TYPE>(stackDepth), std::forward<const UBaseType_t>(priority), std::forward<void(*)(Ts...)>(taskFn));
}



// Thread-safe global object container
template <typename T>
class Global {
    volatile T obj;
    SemaphoreHandle_t lock;
    StaticSemaphore_t lockBuf;
public:
    // Default-construct global object
    Global()
        : obj{}
        , lock(xSemaphoreCreateMutexStatic(&lockBuf))
    {
        xSemaphoreGive(lock);
    }
    
    // Emplace global object
    template <typename... Ts>
    Global(Ts&&... args)
        : obj{std::forward<Ts>(args)...}
        , lock(xSemaphoreCreateMutexStatic(&lockBuf))
    {
        xSemaphoreGive(lock);
    }

    // Writing to the global object requires mutex acquisition
    Global<T>& operator = (T&& rhs) {
        xSemaphoreTake(lock, portMAX_DELAY);
        obj = std::forward<T>(rhs);
        xSemaphoreGive(lock);
        return *this;
    }

    // Any thread can obtain a read-only reference at any time
    operator const volatile T& () const {
        return obj;
    }
};