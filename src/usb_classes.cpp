#include "usb_classes.h"

#include <unordered_map>
#include <vector>

#include "flashdisk.h"
#include "cdcusb.h"
#include "state.h"

#if CFG_TUD_MSC

volatile bool usbMounted = false;

FlashUSB USBStorage;

template <size_t N>
constexpr size_t len(const char (&str)[N]) {
    return N;
}

const char l1[] = "ffat";
char lblBuf1[len(l1)];

bool initMSC()
{
    memcpy(lblBuf1, l1, len(l1));
    if (USBStorage.init("/fat1", lblBuf1))
    {
        return USBStorage.begin();
    }
    return false;
}

#endif

#if CFG_TUD_CDC

CDCusb USBSerial;

class MyCDCCallbacks : public CDCCallbacks {
    void onCodingChange(cdc_line_coding_t const* p_line_coding)
    {
        int bitrate = USBSerial.getBitrate();
        // Serial.printf("new bitrate: %d\n", bitrate);
    }

    bool onConnect(bool dtr, bool rts)
    {
        if (Shell::serialConnectCB)
            Shell::serialConnectCB->invoke();
        return true;  // allow to persist reset, when Arduino IDE is trying to enter bootloader mode
    }

    void onData()
    {
        // Serial.printf("\nnew data, len %d\n", len);
        Shell::serialDataCB();
    }

    void onWantedChar(char c)
    {
        // Serial.printf("wanted char: %c\n", c);
    }
};

const char l2[] = "Mouseless Debug";
char lblBuf2[len(l2)];

bool initSerial()
{
    USBSerial.setCallbacks(new MyCDCCallbacks());
    // USBSerial.setWantedChar('x');

    memcpy(lblBuf2, l2, len(l2));
    return USBSerial.begin(lblBuf2);
}

#endif

class MyUSBCallbacks : public USBCallbacks {
    void onMount() {
        usbMounted = true;
    }

    void onUnmount() {
        usbMounted = false;
    }
};

void initUSB() {
    EspTinyUSB::registerDeviceCallbacks(new MyUSBCallbacks());
}