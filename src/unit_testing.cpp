#include "unit_testing.h"

#include "usb_classes.h"

std::unordered_map<std::string, BaseCallback*> UnitTest::testCbRegistry;

void UnitTest::run(std::vector<const char*>& args) {
    if (args.size() != 1) {
        USBSerial.println("Expected 1 argument");
        return;
    }
    auto testCb = testCbRegistry.find(std::string(args.front()));
    if (testCb == testCbRegistry.end()) {
        USBSerial.printf("Unit test '%s' not found\n", args.front());
        return;
    }
    testCb->second->invoke();
}