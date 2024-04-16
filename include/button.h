#pragma once

#include <Arduino.h>

#include "virtual_callback.h"

#define DEBOUNCE_DURATION pdMS_TO_TICKS(10)
#define LONG_PRESS_DURATION pdMS_TO_TICKS(500)

/// @brief Represents a physical (mechanical) button connected to a GPIO pin; performs debouncing.
class Button {
public:
    /// @brief The kinds of events understood by the `Button` class
    enum class Event : size_t {
        PRESS,   // Occurs at the moment the button is pressed down
        RELEASE, // Occurs at the moment the button is released regardless of how long it was held down; immediately after duration-specific events
        CLICK,   // Occurs at the moment the button is released if it was held down for less time than `LONG_PRESS_DURATION`
        HOLD,    // Occurs at the moment the button is released if it was held down for longer than `LONG_PRESS_DURATION`
        EnumSize // Needs to be the last element of the enum class; it will hold the enum's cardinality
    };

private:
    const int pin;
    bool state;
    TickType_t pressTimestamp;
    BaseCallback *callbacks[(size_t)Event::EnumSize];
    TimerHandle_t debounceTimer;

    /// @brief An interrupt service routine shared by all `Button` instances; it will immediately be executed when a `Button` instance's pin changes state
    /// @param arg A pointer to the instance of `Button` that `ISR` should handle
    static void IRAM_ATTR ISR(void *arg);

    /// @brief A timer callback function shared by all `Button` instances matching `TimerCallbackFunction_t`.
    /// It will be invoked `DEBOUNCE_DURATION` ticks after `ISR`
    /// @param timer 
    static void DebounceCB(TimerHandle_t timer);

    /// @brief A deferred callback invoker matching `PendedFunction_t`
    /// @param instance A pointer to the instance of `Button` for which to invoke a callback
    /// @param cbIdx An index into the target `Button` instance's `callbacks` array specifying which callback to invoke
    static void cbInvoker(void *instance, uint32_t cbIdx);

    /// @brief Check whether the state of a button has changed and invoke any relevant callbacks.
    /// Callbacks will always be invoked by the FreeRTOS timer daemon task.
    /// @param callTime The time in FreeRTOS ticks at which `poll` is being called
    /// @param inISR Whether `poll` is being invoked from an interrupt context; if so, any requisite callbacks will be deferred to a task context using `cbInvoker` via `xTimerPendFunctionCallFromISR`.
    /// @return Whether the button's state has changed since the previous invocation of `poll`
    bool IRAM_ATTR poll(const TickType_t callTime, const bool inISR);

public:
    /// @brief Create an object representing a button connected to a GPIO pin. `Button::attach()` must be called later to attach the `Button` handlers to the GPIO pin.
    /// @param pin The GPIO pin number to which the button is connected
    Button(int pin);
    ~Button();

    /// @brief Attach the `Button` object to its GPIO pin. This functionality is separate from the constructor because GPIO functions are not available until `void setup()` is invoked.
    void attach();

    /// @brief Register a callback function to be executed when a particular `Button::Event` occurs
    /// @tparam F The type of the callback function; it need only be an invocable object. This argument should be deduced automatically.
    /// @param e The event upon which the callback function should be executed
    /// @param callbackFn The callback function to execute
    /// @return An lvalue reference to the same `Button` instance; calls to `Button::on` can be chained for nice formatting
    template <typename F>
    Button& on(Event e, F&& callbackFn) {
        callbacks[(size_t)e] = new Callback<F>(std::forward<F>(callbackFn));
        return *this;
    }
};