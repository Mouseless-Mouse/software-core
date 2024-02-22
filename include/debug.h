#pragma once

#include <Arduino.h>

// Commenting this out will disable all `msg_assert`s (and their performance impacts)
#define DO_DEBUG

#ifdef DO_DEBUG
#define msg_assert(condition, fmt_msg, ...) msg_assert_impl((condition), __PRETTY_FUNCTION__, (fmt_msg), ## __VA_ARGS__)
#else
#define msg_assert(condition, fmt_msg, ...) do {} while(0)
#endif

template <typename... Ts>
void msg_assert_impl(bool condition, const char *fn_sig, const char *fmt_msg, Ts... fmt_args) {
    if (!condition) {
        Serial.printf("%s - ", fn_sig);
        Serial.printf(fmt_msg, fmt_args...);
        delay(2000);
        abort();
    }
}