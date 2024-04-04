#pragma once

#include <Arduino.h>
#include "driver/touch_pad.h"

namespace TouchPads {

static void IRAM_ATTR ISR(void *arg);

static class Status {
    uint32_t bits;
public:
    friend void ISR(void *arg);
    uint32_t getBits() const;
    bool operator[] (size_t i) const;
} status;

static void IRAM_ATTR ISR(void *arg) {
    (void) arg;
    status.bits = touch_pad_get_status();
}

template <touch_pad_t... CHANNELS>
void init(uint32_t threshold) {
    touch_pad_init();
    esp_err_t configDummy[sizeof...(CHANNELS)] = {touch_pad_config(CHANNELS)...};
    esp_err_t threshDummy[sizeof...(CHANNELS)] = {touch_pad_set_thresh(CHANNELS, threshold)...};

    touch_filter_config_t filter_info = {
        .mode = TOUCH_PAD_FILTER_IIR_16,
        .debounce_cnt = 1,
        .noise_thr = 0,
        .jitter_step = 4,
        .smh_lvl = TOUCH_PAD_SMOOTH_IIR_2,
    };
    touch_pad_filter_set_config(&filter_info);
    touch_pad_filter_enable();

    touch_pad_timeout_set(true, SOC_TOUCH_PAD_THRESHOLD_MAX);
    touch_pad_isr_register(ISR, NULL, TOUCH_PAD_INTR_MASK_ALL);
    touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE | TOUCH_PAD_INTR_MASK_TIMEOUT);

    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    touch_pad_fsm_start();
}

void deinit();

};  // end namespace TouchPads