#pragma once

#include <Arduino.h>
#include <utility>
#include "int_seq.h"

typedef void* ParameterPtr;

template <typename F>
struct TaskContainer {
    TaskHandle_t handle;
    F taskFn;
    const char *name;
    const configSTACK_DEPTH_TYPE stackDepth;
    const UBaseType_t priority;
    bool isRunning;
    struct IParamTuple {
        virtual ~IParamTuple() {}
    };
    template <typename... Ts>
    struct ParamTuple : public IParamTuple {
        std::tuple<Ts...> tup;

        ParamTuple(Ts... elements)
            : tup(elements...)
        {}
    };

    IParamTuple *params;

    TaskContainer(const char *name, const configSTACK_DEPTH_TYPE stackDepth, const UBaseType_t priority, F taskFn)
        : handle(nullptr)
        , taskFn(taskFn)
        , name(name)
        , stackDepth(stackDepth)
        , priority(priority)
        , isRunning(false)
        , params(nullptr)
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
        template <typename... Ts>
        static inline void apply(void *instPtr) {
            TaskContainer<F>* inst = reinterpret_cast<TaskContainer<F>*>(instPtr);
            inst->taskFn(std::get<Is>(reinterpret_cast<ParamTuple<Ts...>*>(inst->params)->tup) ...);    // dynamic_cast not permitted with -fno-rtti
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
    
    template <typename... Ts>
    bool operator () (Ts... args) {
        if (params) delete params;
        params = new ParamTuple<Ts...>{args...};
        return isRunning = pdPASS == xTaskCreatePinnedToCore(
            // Get a function pointer to the instantiation of `ApplyWrapper::apply(void *inst)` that matches and can unpack the `params` tuple
            decltype(wrap(std::declval<std14::index_sequence_for<Ts...>>()))::template apply<Ts...>,
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
            params = nullptr;
        }
    }
};

// Factory function to work around the lack of CTAD in C++14
template <typename F>
TaskContainer<F> Task(const char *&&name, const configSTACK_DEPTH_TYPE &&stackDepth, const UBaseType_t &&priority, F&& taskFn) {
    return TaskContainer<F>(std::forward<const char*>(name), std::forward<const configSTACK_DEPTH_TYPE>(stackDepth), std::forward<const UBaseType_t>(priority), std::forward<F>(taskFn));
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