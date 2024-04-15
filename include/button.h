#pragma once

#include <Arduino.h>

#define DEBOUNCE_DURATION pdMS_TO_TICKS(10)
#define LONG_PRESS_DURATION pdMS_TO_TICKS(500)

class BaseCallback {
public:
    virtual void invoke() = 0;
};

template <typename F>
class Callback : public BaseCallback {
    F callbackFunc;

public:
    Callback(F&& callbackFunc)
        : BaseCallback()
        , callbackFunc{std::forward<F>(callbackFunc)}
    {}
    void invoke() {
        callbackFunc();
    }
};

class Button {
public:
    enum class Event : size_t {
        PRESS,
        RELEASE,
        CLICK,
        HOLD,
        EnumSize
    };

private:
    const int pin;
    bool state;
    TickType_t pressTimestamp;
    BaseCallback *callbacks[(size_t)Event::EnumSize];
    TimerHandle_t debounceTimer;
    static void ISR(void *arg);
    static void DebounceCB(TimerHandle_t timer);
    static void cbInvoker(void *instance, uint32_t cbIdx);
    bool poll(const TickType_t callTime, const bool inISR);

public:
    Button(int pin);
    ~Button();

    void attach();

    template <typename F>
    Button& on(Event e, F&& callbackFn) {
        callbacks[(size_t)e] = new Callback<F>(std::forward<F>(callbackFn));
        return *this;
    }
};