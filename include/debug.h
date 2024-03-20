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

/// @brief If a condition is true, print an error message to Serial and enter an infinite loop.
/// @param condition The condition to check
/// @param error_msg The error message to print to Serial
/// @param filename Optionally, the name of the file where the error was encountered
/// @param lineno Optionally, the line number where the error was encountered
void maybe_error(bool condition, const char *error_msg, const char *filename = nullptr,
                              std::size_t lineno = 0) noexcept;
/// @brief If a condition is true, print a warning message to Serial.
/// @param condition The condition to check
/// @param warning_msg The warning message to print to Serial
/// @param filename Optionally, the name of the file where the warning was triggered
/// @param lineno Optionally, the line number where the warning was triggered
/// @return Whether or not the warning was triggered
bool maybe_warn(bool condition, const char *warning_msg, const char *filename = nullptr,
                std::size_t lineno = 0) noexcept;