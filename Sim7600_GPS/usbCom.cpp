#include "usbCom.h"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

// Constructor (Creating UsbCom Object, provides port ["ttyUSB2"] and baudrate [115200])
UsbCom::UsbCom(const std::string &device, int baudRate)
    : device_(device), baudRate_(baudRate), fd_(-1) {}

// Destructor
UsbCom::~UsbCom()
{
    closePort();
}

/**
 * Trys to open the port provided
 *
 * @return false if failed connnection, or true if successful.
 */
bool UsbCom::openPort()
{
    fd_ = open(device_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0)
    {
        std::cerr << "Failed to open " << device_ << ": " << strerror(errno) << '\n';
        return false;
    }

    try
    {
        configureSerial();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        closePort();
        return false;
    }

    return true;
}

/**
 * Closes the object's device (port)
 */
void UsbCom::closePort()
{
    if (fd_ >= 0)
    {
        close(fd_);
        fd_ = -1;
    }
}

/**
 * Closes the object's device (port)
 *
 * Used as part of the openPort function
 */
void UsbCom::configureSerial()
{
    termios tty{};
    if (tcgetattr(fd_, &tty) != 0)
    {
        throw std::runtime_error(std::string("tcgetattr failed: ") + strerror(errno));
    }

    cfmakeraw(&tty);

    speed_t speed = B115200;
    // Spped based on Baudrate for port/device
    // Change speed depending on baudrate (115200 for Sim7600)
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0)
    {
        throw std::runtime_error(std::string("tcsetattr failed: ") + strerror(errno));
    }
}

/**
 * Trys to open the port provided
 *
 * @return false if failed connnection, or true if successful.
 */
bool UsbCom::sendAT()
{
    if (fd_ < 0)
        return false;

    const char *cmd = "AT\r";
    ssize_t n = write(fd_, cmd, strlen(cmd));
    if (n < 0)
    {
        std::cerr << "Write failed: " << strerror(errno) << '\n';
        return false;
    }

    return true;
}

bool UsbCom::waitForOK(int timeoutSeconds)
{
    if (fd_ < 0)
        return false;

    std::string received;
    char buffer[256];

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd_, &readfds);

        timeval timeout;
        timeout.tv_sec = timeoutSeconds;
        timeout.tv_usec = 0;

        int ret = select(fd_ + 1, &readfds, nullptr, nullptr, &timeout);
        if (ret < 0)
        {
            std::cerr << "select failed: " << strerror(errno) << '\n';
            return false;
        }

        if (ret == 0)
        {
            std::cout << "No reply yet, resending AT...\n";
            received.clear();
            if (!sendAT())
                return false;
            continue;
        }

        ssize_t n = read(fd_, buffer, sizeof(buffer) - 1);
        if (n < 0)
        {
            std::cerr << "Read failed: " << strerror(errno) << '\n';
            return false;
        }

        if (n > 0)
        {
            buffer[n] = '\0';
            received += buffer;
            std::cout << buffer << std::flush;

            if (received.find("OK") != std::string::npos)
            {
                return true;
            }
        }
    }
}