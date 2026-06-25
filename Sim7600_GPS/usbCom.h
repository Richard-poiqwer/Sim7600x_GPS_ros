#ifndef USBCOM_H
#define USBCOM_H

#include <string>

class UsbCom
{
public:
    UsbCom(const std::string &device, int baudRate = 115200);
    ~UsbCom();

    bool openPort();
    void closePort();
    bool sendAT();
    bool waitForOK(int timeoutSeconds = 3);

private:
    std::string device_;
    int baudRate_;
    int fd_;

    void configureSerial();
};

#endif
