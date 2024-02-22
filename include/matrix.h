#pragma once

#include <Arduino.h>
#include <initializer_list>

template <typename T, size_t H, size_t W>
class Array {
    T data[H][W];
    template <typename U, size_t D1, size_t D2>
    friend class Array;

public:
    Array() : data{0} {}
    // Array(T(& vals)[H][W]) : data(vals) {}
    Array(const std::initializer_list<const std::initializer_list<T>> arr) : data{0} {
        size_t i = 0;
        for (const std::initializer_list<T> *it = arr.begin(); it != arr.end(); ++it) {
            size_t j = 0;
            for (const T *jt = it->begin(); jt != it->end(); ++jt) {
                data[i][j++] = *jt;
            }
            ++i;
        }
    }

    // template <size_t X = W, typename... Ts, typename = typename std::enable_if<X == 1 && sizeof...(Ts) == H, void>::type>
    // Array(Ts... vals) : data{vals...} {}

    template <typename U, size_t W2>
    Array<decltype(std::declval<T>() * std::declval<U>()), H, W2> operator * (const Array<U, W, W2> &rhs) const {
        typedef decltype(std::declval<T>() * std::declval<U>()) return_t;
        Array<return_t, H, W2> result;
        for (size_t row = 0; row < H; ++row)
            for (size_t col = 0; col < W2; ++col) {
                return_t sum = 0;
                for (size_t i = 0; i < W; ++i)
                    sum += this->data[row][i] * rhs.data[i][col];
                result.data[row][col] = sum;
            }
        return result;
    }

    template <size_t X = W>
    typename std::enable_if<X != 1, T>::type (&operator [] (size_t i)) [W] {
        return data[i];
    }

    template <size_t X = W>
    typename std::enable_if<X != 1, const T>::type (&operator [] (size_t i) const) [W] {
        return data[i];
    }

    template <size_t X = W>
    typename std::enable_if<X == 1, T>::type &operator [] (size_t i) {
        return data[i][0];
    }

    template <size_t X = W>
    typename std::enable_if<X == 1, const T>::type &operator [] (size_t i) const {
        return data[i][0];
    }
};

template <typename T, size_t H>
using Vector = Array<T, H, 1>;

template <typename T>
Array<T, 4, 4> translate(const Vector<T, 3> &vec) {
    Array<T, 4, 4> result{
        {1, 0, 0, vec[0]},
        {0, 1, 0, vec[1]},
        {0, 0, 1, vec[2]},
        {0, 0, 0, 1}
    };
    return result;
}

Array<float, 4, 4> rotX(const float theta) {
    Array<float, 4, 4> result{
        {1, 0,          0,           0},
        {0, cos(theta), -sin(theta), 0},
        {0, sin(theta), cos(theta),  0},
        {0, 0,          0,           1}
    };
    return result;
}

Array<float, 4, 4> rotY(const float theta) {
    Array<float, 4, 4> result{
        {cos(theta),  0, sin(theta), 0},
        {0,           1, 0,          0},
        {-sin(theta), 0, cos(theta), 0},
        {0,           0, 0,          1}
    };
    return result;
}

Array<float, 4, 4> rotZ(const float theta) {
    Array<float, 4, 4> result{
        {cos(theta), -sin(theta), 0, 0},
        {sin(theta), cos(theta),  0, 0},
        {0, 0,                    1, 0},
        {0, 0,                    0, 1}
    };
    return result;
}