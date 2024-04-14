#include "debug.h"
#include <Arduino.h>
#include "usb_classes.h"

void maybe_error(bool condition, const char *error_msg, const char *filename, std::size_t lineno) noexcept
{
    if (!condition)
    {
        return;
    }

    USBSerial.print("ERROR ENCOUNTERED");
    if (filename != nullptr)
    {
        USBSerial.printf(" IN FILE \"%s\"", filename);
    }
    if (lineno != 0)
    {
        USBSerial.printf(" AT LINE %u", lineno);
    }
    USBSerial.printf(": %s\n", error_msg);

    for (;;)
    {
    }
}

bool maybe_warn(bool condition, const char *warning_msg, const char *filename, std::size_t lineno) noexcept
{
    if (!condition)
    {
        return condition;
    }

    USBSerial.print("WARNING EMITTED");
    if (filename != nullptr)
    {
        USBSerial.printf(" IN FILE \"%s\"", filename);
    }
    if (lineno != 0)
    {
        USBSerial.printf(" AT LINE %u", lineno);
    }
    USBSerial.printf(": %s\n", warning_msg);

    return condition;
}