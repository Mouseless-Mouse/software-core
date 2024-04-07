#pragma once

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

}; // end namespace meta