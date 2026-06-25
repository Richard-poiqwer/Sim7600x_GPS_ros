#include "usbCom.h"
#include <iostream>

int main()
{
    UsbCom usb("/dev/ttyUSB2", 115200);

    if (!usb.openPort())
    {
        return 1;
    }

    if (!usb.sendAT())
    {
        return 1;
    }

    if (usb.waitForOK(3))
    {
        std::cout << "\nReceived OK once, stopping.\n";
    }

    return 0;
}