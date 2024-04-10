#include "touch.h"

uint32_t TouchPads::Status::getBits() const {
    return bits;
}

bool TouchPads::Status::operator[] (size_t i) const {
    return bits >> i & 1;
}

void TouchPads::deinit() {
    touch_pad_isr_deregister(ISR, NULL);
    touch_pad_deinit();
}