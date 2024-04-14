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
        // if (dtr && rts) {
        //     USBSerial.persistentReset(RESTART_BOOTLOADER);
        //     esp_restart();
        // }
            
        // USBSerial.printf("connection state changed, dtr: %d, rts: %d\n", dtr, rts);
        return true;  // allow to persist reset, when Arduino IDE is trying to enter bootloader mode
    }

    void onData()
    {
        // Serial.printf("\nnew data, len %d\n", len);
        Shell::serialCB();
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

const char mousey[] = "         %s\n         /\n(\\   /) /  _\n (0 0)____  \\\n \"\\ /\"    \\ /\n  |' ___   /\n   \\/   \\_/\n";

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
    Shell::registerCmd("mousesay", [](std::vector<const char*>& args){
        std::string result;
        for (const char *arg : args) {
            result += arg;
            result += " ";
        }
        if (!result.empty())
            result.pop_back();
        USBSerial.printf(mousey, result.c_str());
    });
}