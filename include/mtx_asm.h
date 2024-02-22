#pragma once

#include <Arduino.h>
#include "debug.h"

// The number of bits of the fixed-point representation that go to the right of the decimal point (trade-off between range and precision)
#define FIXED16_FRACTIONAL_BITS 12   // Range of [-8.0, 8.0) in steps of 1/4096

// Calculated from FIXED16_FRACTIONAL_BITS (don't change this definition; change the one above)
#define FIXED16_INT_MULTIPLIER ((float)(1 << FIXED16_FRACTIONAL_BITS))

// Fixed-point 16-bit fractional number structure
struct fp16_t {
    int16_t val;
private:
    constexpr fp16_t (float flt, bool ign) : val(FIXED16_INT_MULTIPLIER * flt) {}
public:
    friend constexpr fp16_t operator ""_fp16 (long double n);
    fp16_t (float flt) : val(FIXED16_INT_MULTIPLIER * flt) {
        msg_assert(abs(flt) < 8.f, "Out-of-range initial value (got %f)", flt);
    }
    fp16_t() : fp16_t(0.f) {}
    fp16_t &operator = (float rhs) { 
        val = FIXED16_INT_MULTIPLIER * rhs;
        msg_assert(abs(rhs) < 8.f, "Out-of-range assignment (got %f)", rhs);
        return *this;
    }
    operator float () const {
        // msg_assert(val != 32767 && val != -32768, "Possible fixed-point overflow (internal value is %i)", val);
        return val / FIXED16_INT_MULTIPLIER;
    }
    operator float () const volatile {
        // msg_assert(val != 32767 && val != -32768, "Possible fixed-point overflow (internal value is %i)", val);
        return val / FIXED16_INT_MULTIPLIER;
    }
};

constexpr fp16_t operator ""_fp16 (const long double n) {
    return fp16_t(n, false);
}

// Vector of `fp16_t`s with 4 rows
struct Vec4_fp16 {
    fp16_t data[4];

    Vec4_fp16() : data{0._fp16, 0._fp16, 0._fp16, 0._fp16} {}
    Vec4_fp16(float a, float b, float c, float d) : data{a, b, c, d} {}

    fp16_t &operator [] (const size_t i) { return data[i]; }
    volatile fp16_t &operator [] (const size_t i) volatile { return data[i]; }
    const volatile fp16_t &operator [] (const size_t i) const volatile { return data[i]; }
};

void mul_vector_list_scalar(volatile Vec4_fp16 *list, volatile Vec4_fp16 *dest, fp16_t mul, uint32_t len);

// Matrix of `fp16_t`s with 4 rows and 4 columns
class Mat4_fp16 {
    fp16_t data[16] __attribute__((aligned (16)));

public:
    class RowReference {
        fp16_t *data;
        ptrdiff_t offset;
        RowReference(fp16_t *data, ptrdiff_t offset);

    public:
        friend class Mat4_fp16;
        friend void mul_mtx_list_scalar(volatile Mat4_fp16 *list, volatile Mat4_fp16 *dest, fp16_t mul, uint32_t len);
        fp16_t &operator [] (size_t i);
        RowReference &operator ++ ();
        bool operator != (RowReference &rhs) const;
        RowReference &operator * ();
    };
    Mat4_fp16();
    Mat4_fp16(
        float a, float b, float c, float d,
        float e, float f, float g, float h,
        float i, float j, float k, float l,
        float m, float n, float o, float p
    );

    Mat4_fp16(const Mat4_fp16 &other);
    Mat4_fp16 &operator = (const Mat4_fp16 &other);
    RowReference operator [] (size_t i);
    RowReference begin();
    RowReference end();

    // TODO: Allocate contiguous memory for data of a list of matrices
    void *operator new[] (size_t count);

    void mul_vector_list(volatile Vec4_fp16 *list, volatile Vec4_fp16 *dest, uint32_t len) const;
    Mat4_fp16 operator * (const Mat4_fp16 &rhs) const;
    Vec4_fp16 operator * (const Vec4_fp16 &rhs) const;

    void print();
};

Mat4_fp16 translate_fp16(const float x, const float y, const float z);

Mat4_fp16 rotX_fp16(const float theta);

Mat4_fp16 rotY_fp16(const float theta);

Mat4_fp16 rotZ_fp16(const float theta);