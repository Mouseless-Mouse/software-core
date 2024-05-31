#pragma once

#include <Arduino.h>
#include <unordered_map>
#include <vector>

#include "virtual_callback.h"

namespace UnitTest {

extern std::unordered_map<std::string, BaseCallback*> testCbRegistry;

/// @brief Register an invocable object as a unit test to be made available via the `test` shell command.
/// @tparam F The underlying type of the invocable object; this should be deduced automatically.
/// @param name The name by which the unit test will be registered (i.e. `test <name>` will run the test)
/// @param invocable The invocable object containing the unit test code
template <typename F>
void add(std::string&& name, F&& invocable) {
    testCbRegistry.emplace(
        std::forward<std::string>(name), new Callback<F>(std::forward<F>(invocable))
    );
}

void run(const std::vector<const char*>& args);

};  // namespace UnitTest

/// @brief The primary template for `class Profiler`. This template will never be instantiated because every well-formed
/// SFINAE will match (exactly) one of the two partial template specializations.
/// @tparam T The type of object deriving from `Print` of which `Profiler<T>` will store either an instance or a reference to an instance
/// @tparam void SFINAE restricting `Profiler<T>` instantiations to only `T` derived from `Print`
/// @tparam std::true_type||std::false_type Whether T is an lvalue reference. Selects which partial template specialization is instantiated.
template <typename T, typename = typename std::enable_if<std::is_base_of<Print, typename std::remove_reference<T>::type>::value, void>::type, typename = typename std::is_lvalue_reference<T>::type>
class Profiler {};

/// @brief Implementation of `Profiler` for persistent instances derived from `Print` (e.g. `USBSerial`, `display`).
/// Does not destroy the `Print` object upon its own destruction.
/// @tparam T The type of the object deriving from `Print`
template <typename T>
class Profiler<T, void, std::true_type> {
    T out;  // T is an lvalue reference
    TickType_t startTime;

public:
    Profiler(T out)
        : out(out)
        , startTime(xTaskGetTickCount())
    {}
    
    ~Profiler() {
        out.println(xTaskGetTickCount() - startTime);
    }
};

/// @brief Implementation of `Profiler` for temporary instances derived from `Print` (e.g. `TaskPrint`, `TaskLog`).
/// Destroys the `Print` object upon its own destruction.
/// @tparam T The type of the object deriving from `Print`
template <typename T>
class Profiler<T, void, std::false_type> {
    T out;  // T is an lvalue
    TickType_t startTime;

public:
    Profiler(T&& out)
        : out(std::forward<T>(out))
        , startTime(xTaskGetTickCount())
    {}
    
    ~Profiler() {
        out.println(xTaskGetTickCount() - startTime);
    }
};

/// @brief Profiler factory function. Deduces the type of `out`, including whether it is an lvalue or an rvalue.
/// @tparam T The type of `out`, which must derive from `Print` for instantiation of `Profiler<T&>` to succeed
/// @param out An lvalue reference to an instance of an object deriving from `Print`
/// @return An instance of `Profiler<T&>` that will print its lifetime in FreeRTOS ticks via `out` upon its destruction
template <typename T>
Profiler<T&> Profile(T& out) { return Profiler<T&>(out); }

/// @brief Profiler factory function. Deduces the type of `out`, including whether it is an lvalue or an rvalue.
/// @tparam T The type of `out`, which must derive from `Print` for instantiation of `Profiler<T>` to succeed
/// @param out An rvalue reference to an instance of an object deriving from `Print`
/// @return An instance of `Profiler<T>` that will print its lifetime in FreeRTOS ticks via `out` upon its destruction
template <typename T>
Profiler<T> Profile(T&& out) { return Profiler<T>(std::forward<T>(out)); }