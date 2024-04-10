#include "usb_classes.h"

#include "flashdisk.h"
#include "cdcusb.h"

#if CFG_TUD_MSC

FlashUSB USBStorage;

template <size_t N>
constexpr size_t len(const char (&str)[N]) {
    return N;
}

const char l1[] = "ffat";
char lblBuf[len(l1)];

bool initMSC()
{
    memcpy(lblBuf, l1, len(l1));
    if (USBStorage.init("/fat1", lblBuf))
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
        Serial.printf("new bitrate: %d\n", bitrate);
    }

    bool onConnect(bool dtr, bool rts)
    {
        Serial.printf("connection state changed, dtr: %d, rts: %d\n", dtr, rts);
        return true;  // allow to persist reset, when Arduino IDE is trying to enter bootloader mode
    }

    void onData()
    {
        int len = USBSerial.available();
        Serial.printf("\nnew data, len %d\n", len);
        uint8_t buf[len] = {};
        USBSerial.read(buf, len);
        Serial.write(buf, len);
    }

    void onWantedChar(char c)
    {
        Serial.printf("wanted char: %c\n", c);
    }
};

bool initSerial()
{
    // USBSerial.setCallbacks(new MyCDCCallbacks());
    // USBSerial.setWantedChar('x');

    return USBSerial.begin("Mouseless Debug");
}

#endif