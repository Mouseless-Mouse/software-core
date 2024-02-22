#include "mtx_asm.h"
#include "debug.h"

// Constructors for Mat4_fp16

Mat4_fp16::Mat4_fp16() : data{0}
{}

Mat4_fp16::Mat4_fp16(
    float a, float b, float c, float d,
    float e, float f, float g, float h,
    float i, float j, float k, float l,
    float m, float n, float o, float p
) : data{a,e,i,m,b,f,j,n,c,g,k,o,d,h,l,p}
{}

Mat4_fp16 &Mat4_fp16::operator = (const Mat4_fp16 &other) {
    memcpy(this->data, other.data, 16 * sizeof(fp16_t));
    return *this;
}

Mat4_fp16::Mat4_fp16(const Mat4_fp16 &other) : Mat4_fp16() {
    *this = other;
}



// RowReference definitions

Mat4_fp16::RowReference::RowReference(fp16_t *data, ptrdiff_t offset) : data(data), offset(offset)
{}

fp16_t &Mat4_fp16::RowReference::operator [] (size_t i) {
    return *(data + 4*i + offset);
}

Mat4_fp16::RowReference Mat4_fp16::operator [] (size_t i) {
    return RowReference(data, i);
}

Mat4_fp16::RowReference Mat4_fp16::begin() {
    return RowReference(data, 0);
}

Mat4_fp16::RowReference Mat4_fp16::end() {
    return RowReference(data, 4);
}

Mat4_fp16::RowReference &Mat4_fp16::RowReference::operator ++ () {
    ++offset;
    return *this;
}

bool Mat4_fp16::RowReference::operator != (Mat4_fp16::RowReference &rhs) const {
    return this->offset != rhs.offset || this->data != rhs.data;
}

Mat4_fp16::RowReference &Mat4_fp16::RowReference::operator * () {
    return *this;
}



// Mat4_fp16 computation and manipulation methods

void Mat4_fp16::mul_vector_list(volatile Vec4_fp16 *list, volatile Vec4_fp16 *dest, uint32_t len) const {
    msg_assert((len & 1) == 0, "Mat4_fp16::mul_vector_list(): Vector list length `len` must be even (got %i)", len);
    msg_assert((((uintptr_t)list | (uintptr_t)dest) & 15) == 0, "Vector source and destination pointers must be aligned on a 16-byte boundary");

    // Create a pointer past the end of the input list
    volatile Vec4_fp16 *end = list + len;
    asm volatile(R"ASM(
        ee.vld.l.64.ip q4, %2, 0        // Set lower half of register q4 to {a e i m}, don't increment address pointer
        ee.vld.h.64.ip q4, %2, 8        // Register q4 now contains {a e i m a e i m}, increment address pointer by 8 bytes
        ee.vld.l.64.ip q5, %2, 0        // Now %2 (register assigned to pointer `slots`) points to {b f j n}
        ee.vld.h.64.ip q5, %2, 8        // Register q5 now contains {b f j n b f j n}, increment address pointer by 8 bytes
        ee.vld.l.64.ip q6, %2, 0
        ee.vld.h.64.ip q6, %2, 8
        ee.vld.l.64.ip q7, %2, 0
        ee.vld.h.64.ip q7, %2, 8

        movi.n %2, %4                   // Pointer to matrix is no longer necessary, so recycle the register to hold the right shift distance for use in the loop

    mtxMult:
        ee.vldhbc.16.incp q0, q1, %0    // Load registers q0 and q1 from `list` such that two Vec4_fp16's {q r s t} and {u v w x} go into q0 and q1 as {q q r r s s t t} and {u u v v w w x x}, respectively, and then increment `list`
        ee.vzip.32 q0, q1               // Zipping 32-bit regions results in {q0, q1} being {q q u u r r v v s s w w t t x x}
        mv.qr q2, q0                    // Copy q0 to q2
        mv.qr q3, q1                    // Copy q1 to q3
        ee.vzip.32 q0, q2               // Zipping 32-bit regions of {q0, q2} results in {q0, q2} being {q q q q u u u u r r r r v v v v}
        ee.vzip.32 q1, q3               // Same process on {q1, q3} yields {s s s s w w w w t t t t x x x x} - note that the vector components are in {q0, q2, q1, q3}, not {q0, q1, q2, q3}

        ee.zero.qacc                    // Clear the gigantic partitioned vector accumulator
        ee.vmulas.s16.qacc q0, q4       // Multiply q0 and q4 ({q q q q u u u u} and {a e i m a e i m}), add result of each multiplication to its respective slice of the accumulator
        ee.vmulas.s16.qacc q2, q5       // Multiply q2 and q5 ({r r r r v v v v} and {b f j n b f j n}), accumulate (QACC now contains {qa+rb qe+rf qi+rj qm+rn ua+vb ue+vf ui+vj um+vn})
        ee.vmulas.s16.qacc q1, q6       // QACC now contains {qa+rb+sc ... qm+rn+so ua+vb+wc ... um+vn+wo}
        ee.vmulas.s16.qacc q3, q7       // QACC now contains {qa+rb+sc+td ... qm+rn+so+tp ua+vb+wc+xd ... um+vn+wo+xp} which is {q' ... t' u' ... x'}

        ee.srcmb.s16.qacc q0, %2, 0     // Arithmetic right shift each slice of QACC by FIXED16_FRACTIONAL_BITS in accordance with fixed-point representation, store the 16-bit result from each slice into q0 (not needed anymore)
        ee.vst.128.ip q0, %1, 16        // Store the final result held in q0 to `dest`, increment address pointer

        bltu %0, %3, mtxMult            // Loop while the pointer `list` is less than the address `end`
    )ASM"
    :
    : "r"(list), "r"(dest), "r"(data), "r"(end), "I"(FIXED16_FRACTIONAL_BITS)
    // : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7"
    );
}

Mat4_fp16 Mat4_fp16::operator * (const Mat4_fp16 &rhs) const {
    Mat4_fp16 result;
    mul_vector_list((volatile Vec4_fp16*)(rhs.data), reinterpret_cast<volatile Vec4_fp16*>(result.data), 4);
    return result;
}

Vec4_fp16 Mat4_fp16::operator * (const Vec4_fp16 &rhs) const {
    Vec4_fp16 result;
    result[0].val = data[0].val * rhs[0].val + data[1].val * rhs[1].val + data[2].val * rhs[2].val + data[3].val * rhs[3].val >> FIXED16_FRACTIONAL_BITS;
    result[1].val = data[4].val * rhs[0].val + data[5].val * rhs[1].val + data[6].val * rhs[2].val + data[7].val * rhs[3].val >> FIXED16_FRACTIONAL_BITS;
    result[2].val = data[8].val * rhs[0].val + data[9].val * rhs[1].val + data[10].val * rhs[2].val + data[11].val * rhs[3].val >> FIXED16_FRACTIONAL_BITS;
    result[3].val = data[12].val * rhs[0].val + data[13].val * rhs[1].val + data[14].val * rhs[2].val + data[15].val * rhs[3].val >> FIXED16_FRACTIONAL_BITS;
    return result;
}

void Mat4_fp16::print() {
    for (RowReference &row : *this) {
        Serial.printf("| % .3f % .3f % .3f % .3f |\n", (float)row[0], (float)row[1], (float)row[2], (float)row[3]);
    }
}

// Multiply a list of vectors by a scalar value
void mul_vector_list_scalar(volatile Vec4_fp16 *list, volatile Vec4_fp16 *dest, fp16_t mul, uint32_t len) {
    msg_assert((len & 1) == 0, "Mat4_fp16::mul_vector_list(): Vector list length `len` must be even (got %i)", len);
    msg_assert((((uintptr_t)list | (uintptr_t)dest) & 15) == 0, "Vector source and destination pointers must be aligned on a 16-byte boundary");

    // Create a pointer past the end of the input list
    volatile Vec4_fp16 *end = list + len;

    asm volatile(R"ASM(
        ee.vldbc.16 q0, %2  // Load 16-bit scalar multiplier into each segment of register q0
        movi.n %2, %4       // Recycle the multiplier address register to hold the right shift distance
        wsr.sar %2          // Move the right shift distance into the shift amount register (SAR)

        ee.vld.128.ip q1, %0, 16                // Load two vectors {a1 b1 c1 d1 a2 b2 c2 d2} into register q1 and increment `list` by 16 bytes
        bgeu %0, %3, vecScalarMultFinal         // To avoid out-of-bounds memory accesses, branch to a non-reading final iteration if `len` was 2

    vecScalarMult:
        ee.vmul.s16.ld.incp q1, %0, q2, q1, q0  // Multiply each slice of q1 and q0, right shift the result by SAR and store in q2, load from `list` into q1 and increment `list` by 16 bytes
        ee.vst.128.ip q2, %1, 16                // Store the result of the multiply-shift operation to `dest` and increment `dest` by 16 bytes

        blt %0, %3, vecScalarMult               // Loop while the pointer `list` is less than the address `end`

    vecScalarMultFinal:
        ee.vmul.s16 q2, q1, q0      // Multiply each slice of q1 and q0, right shift the result by SAR and store in q2
        ee.vst.128.ip q2, %1, 16    // Store the result of the final iteration to `dest` and increment `dest` by 16 bytes
    )ASM"
    :
    : "r"(list), "r"(dest), "r"(&mul), "r"(end), "I"(FIXED16_FRACTIONAL_BITS)
    );
}

// Find the pointwise (Hadamard) product of two vectors ({a b c d} * {e f g h} = {ae bf cg dh})
void mul_vector_list_pointwise(volatile Vec4_fp16 *lhs_list, volatile Vec4_fp16 *rhs_list, volatile Vec4_fp16 *dest, uint32_t len) {
    msg_assert((len & 1) == 0, "Mat4_fp16::mul_vector_list(): Vector list length `len` must be even (got %i)", len);
    msg_assert((((uintptr_t)lhs_list | (uintptr_t)rhs_list | (uintptr_t)dest) & 15) == 0, "Vector source and destination pointers must be aligned on a 16-byte boundary");

    // Create a pointer past the end of the input list
    volatile Vec4_fp16 *end = lhs_list + len;

    asm volatile(R"ASM(
        wsr.sar %4          // Move the right shift distance into the shift amount register (SAR)

        ee.vld.128.ip q0, %0, 16                // Load two vectors from `lhs_list` into register q0 and increment `lhs_list` by 16 bytes
        ee.vld.128.ip q1, %1, 16                // Load two vectors from `rhs_list` into register q1 and increment `rhs_list` by 16 bytes
        bgeu %0, %3, vecPointwiseMultFinal      // To avoid out-of-bounds memory accesses, branch to a non-reading final iteration if `len` was 2

    vecPointwiseMult:
        ee.vmul.s16.ld.incp q0, %0, q2, q1, q0  // Multiply each slice of q1 and q0, right shift the result by SAR and store in q2, load from `lhs_list` into q0 and increment `lhs_list` by 16 bytes
        ee.vld.128.ip q1, %1, 16                // Load two vectors from `rhs_list` into q1 and increment `rhs_list` by 16 bytes
        ee.vst.128.ip q2, %1, 16                // Store the result of the multiply-shift operation to `dest` and increment `dest` by 16 bytes

        blt %0, %3, vecPointwiseMult            // Loop while the pointer `list` is less than the address `end`

    vecPointwiseMultFinal:
        ee.vmul.s16 q2, q1, q0      // Multiply each slice of q1 and q0, right shift the result by SAR and store in q2
        ee.vst.128.ip q2, %2, 16    // Store the result of the final iteration to `dest` and increment `dest` by 16 bytes
    )ASM"
    :
    : "r"(lhs_list), "r"(rhs_list), "r"(dest), "r"(end), "r"(FIXED16_FRACTIONAL_BITS)
    );
}

Mat4_fp16 translate_fp16(const float x, const float y, const float z) {
    Mat4_fp16 result(
        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1
    );
    return result;
}

Mat4_fp16 rotX_fp16(const float theta) {
    Mat4_fp16 result(
        1, 0,          0,           0,
        0, cos(theta), -sin(theta), 0,
        0, sin(theta), cos(theta),  0,
        0, 0,          0,           1
    );
    return result;
}

Mat4_fp16 rotY_fp16(const float theta) {
    Mat4_fp16 result(
        cos(theta),  0, sin(theta), 0,
        0,           1, 0,          0,
        -sin(theta), 0, cos(theta), 0,
        0,           0, 0,          1
    );
    return result;
}

Mat4_fp16 rotZ_fp16(const float theta) {
    Mat4_fp16 result(
        cos(theta), -sin(theta), 0, 0,
        sin(theta), cos(theta),  0, 0,
        0, 0,                    1, 0,
        0, 0,                    0, 1
    );
    return result;
}