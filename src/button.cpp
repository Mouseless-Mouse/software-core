#include "button.h"

Button::Button(int pin)
    : pin(pin)
    , state(false)
    , pressTimestamp(0)
    , callbacks{nullptr}
    , debounceTimer{xTimerCreate("Button Debounce", DEBOUNCE_DURATION, false, this, DebounceCB)}
{}

Button::~Button() {
    detachInterrupt(pin);
}

void Button::attach() {
    pinMode(pin, INPUT);    // Pins are externally pulled HIGH in board schematic
    attachInterruptArg(pin, ISR, this, CHANGE);
}

void Button::ISR(void *arg) {
    Button *button = static_cast<Button*>(arg);
    const TickType_t irqTime = xTaskGetTickCountFromISR();

    button->poll(irqTime, true);

    BaseType_t taskWoken = pdFALSE;
    detachInterrupt(button->pin);
    xTimerResetFromISR(button->debounceTimer, &taskWoken);
    if (taskWoken == pdTRUE)
        portYIELD_FROM_ISR();
}

void Button::DebounceCB(TimerHandle_t timer) {
    Button *button = static_cast<Button*>(pvTimerGetTimerID(timer));
    const TickType_t cbTime = xTaskGetTickCount();

    // Debounce again after another interval of DEBOUNCE_DURATION ticks if the button state has changed since the last poll
    // Otherwise, consider the pin state stable and reattach the ISR
    if (button->poll(cbTime, false))
        xTimerReset(timer, 0);  // The timer task invokes `DebounceCB`, so blocking the timer task until the timer queue is not full would be very bad
    else
        attachInterruptArg(button->pin, ISR, button, CHANGE);
}

void Button::cbInvoker(void *instance, uint32_t cbIdx) {
    static_cast<Button*>(instance)->callbacks[cbIdx]->invoke();
}

bool Button::poll(const TickType_t callTime, const bool inISR) {
    BaseType_t taskWoken = pdFALSE;
    bool pinState = !digitalRead(pin);  // Button is active low

    if (pinState == state)
        return false;
    
    state = pinState;
    if (state) {
        pressTimestamp = callTime;
        if (callbacks[(size_t)Event::PRESS]) {
            if (inISR)
                xTimerPendFunctionCallFromISR(cbInvoker, this, (size_t)Event::PRESS, &taskWoken);
            else
                callbacks[(size_t)Event::PRESS]->invoke();
        }
    }
    else {
        if (callTime - pressTimestamp >= LONG_PRESS_DURATION) {
            if (callbacks[(size_t)Event::HOLD]) {
                if (inISR)
                    xTimerPendFunctionCallFromISR(cbInvoker, this, (size_t)Event::HOLD, &taskWoken);
                else
                    callbacks[(size_t)Event::HOLD]->invoke();
            }
        }
        else {
            if (callbacks[(size_t)Event::CLICK]) {
                if (inISR)
                    xTimerPendFunctionCallFromISR(cbInvoker, this, (size_t)Event::CLICK, &taskWoken);
                else
                    callbacks[(size_t)Event::CLICK]->invoke();
            }
        }
        if (callbacks[(size_t)Event::RELEASE]) {
            if (inISR)
                xTimerPendFunctionCallFromISR(cbInvoker, this, (size_t)Event::RELEASE, &taskWoken);
            else
                callbacks[(size_t)Event::RELEASE]->invoke();
        }
    }
    if (inISR && taskWoken == pdTRUE)
        portYIELD_FROM_ISR();
    return true;
}