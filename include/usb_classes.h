#pragma once

#include <vector>

#include "flashdisk.h"
#include "cdcusb.h"

bool initMSC();
bool initSerial();
void initUSB();

extern FlashUSB USBStorage;
extern CDCusb USBSerial;
extern volatile bool usbMounted;