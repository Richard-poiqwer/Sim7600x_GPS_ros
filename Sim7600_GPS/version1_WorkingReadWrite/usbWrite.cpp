#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

int main()
{
    const char *device = "/dev/ttyUSB2";
    const char *msg = "AT\r";

    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        std::cerr << "Failed to open " << device << ": " << strerror(errno) << '\n';
        return 1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0)
    {
        std::cerr << "tcgetattr failed: " << strerror(errno) << '\n';
        close(fd);
        return 1;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        std::cerr << "tcsetattr failed: " << strerror(errno) << '\n';
        close(fd);
        return 1;
    }

    ssize_t written = write(fd, msg, strlen(msg));
    if (written < 0)
    {
        std::cerr << "write failed: " << strerror(errno) << '\n';
    }
    else
    {
        std::cout << "Sent " << written << " bytes\n";
    }

    close(fd);
    return 0;
}
