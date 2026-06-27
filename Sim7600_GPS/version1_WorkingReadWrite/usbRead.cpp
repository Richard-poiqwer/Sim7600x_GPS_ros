#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

int main()
{
    const char *device = "/dev/ttyUSB2";

    int fd = open(device, O_RDONLY | O_NOCTTY);
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

    tty.c_cc[VMIN] = 1; // Wait for 1 Byte
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        std::cerr << "tcsetattr failed: " << strerror(errno) << '\n';
        close(fd);
        return 1;
    }

    char buffer[256];

    std::cout << "Monitoring " << device << "...\n";

    while (true)
    {
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n < 0)
        {
            std::cerr << "Read failed: " << strerror(errno) << '\n';
            break;
        }
        else if (n == 0)
        {
            continue;
        }

        buffer[n] = '\0';
        std::cout << buffer << std::flush;
    }

    //    std::cout << "Monitoring " << device << " ... press Ctrl+C to stop\n";

    // char buffer[256];
    // while (true) {
    //     ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    //     if (n < 0) {
    //         std::cerr << "Read failed: " << strerror(errno) << '\n';
    //         break;
    //     }

    //     if (n > 0) {
    //         buffer[n] = '\0';
    //         std::cout << buffer << std::flush;
    //     }
    // }

    close(fd);
    return 0;
}