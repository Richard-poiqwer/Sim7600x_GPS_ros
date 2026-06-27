#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

static void configureSerial(int fd)
{
    termios tty{};
    if (tcgetattr(fd, &tty) != 0)
    {
        throw std::runtime_error(std::string("tcgetattr failed: ") + strerror(errno));
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        throw std::runtime_error(std::string("tcsetattr failed: ") + strerror(errno));
    }
}

static bool sendAT(int fd)
{
    const char *cmd = "AT\r";
    ssize_t n = write(fd, cmd, strlen(cmd));
    if (n < 0)
    {
        std::cerr << "Write failed: " << strerror(errno) << '\n';
        return false;
    }
    return true;
}

int main()
{
    const char *device = "/dev/ttyUSB2";

    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        std::cerr << "Failed to open " << device << ": " << strerror(errno) << '\n';
        return 1;
    }

    try
    {
        configureSerial(fd);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        close(fd);
        return 1;
    }

    std::cout << "Sending AT once, then waiting for OK...\n";

    if (!sendAT(fd))
    {
        close(fd);
        return 1;
    }

    std::string received;
    char buffer[256];
    bool gotOK = false;

    while (!gotOK)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int ret = select(fd + 1, &readfds, nullptr, nullptr, &timeout);
        if (ret < 0)
        {
            std::cerr << "select failed: " << strerror(errno) << '\n';
            break;
        }

        if (ret == 0)
        {
            std::cout << "No reply yet, resending AT...\n";
            received.clear();
            if (!sendAT(fd))
            {
                break;
            }
            continue;
        }

        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n < 0)
        {
            std::cerr << "Read failed: " << strerror(errno) << '\n';
            break;
        }

        if (n > 0)
        {
            buffer[n] = '\0';
            received += buffer;
            std::cout << buffer << std::flush;

            if (received.find("OK") != std::string::npos)
            {
                gotOK = true;
            }
        }
    }

    if (gotOK)
    {
        std::cout << "\nReceived OK once, stopping.\n";
    }

    close(fd);
    return 0;
}