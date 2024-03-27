#pragma once

#include "flashdisk.h"
#include "cdcusb.h"

bool initMSC();
bool initSerial();

extern FlashUSB USBStorage;
extern CDCusb USBSerial;