#pragma once

#include "usb_classes.h"

namespace meta {

template <typename... Ts>
struct get_first_type {};

template <typename T, typename... Ts>
struct get_first_type<T, Ts...> {
    typedef T type;
};

template <typename... Ts>
using first_t = typename get_first_type<Ts...>::type;

template <typename... Ts, typename F>
void for_each(F func, Ts&&... args) {
    uint8_t result[sizeof...(Ts)] = {(func(std::forward<Ts>(args)), uint8_t(0))...};
}

template <typename V, typename E> struct result_t {
    union {
        V value;
        E error;
    };
    bool has_error;

    result_t(V value) : value(value), has_error(false) {}
    result_t(E error) : error(error), has_error(true) {}

    V unwrap() {
        if (has_error) {
            USBSerial.printf("Unwrapped error in task %s\n. Aborting task.",
                             pcTaskGetName(nullptr));
            vTaskDelete(nullptr);
        }
        return value;
    }
};

}; // end namespace meta