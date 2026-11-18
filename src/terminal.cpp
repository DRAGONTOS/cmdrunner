#include "includes/terminal.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdexcept>
#include <unistd.h>

terminal::terminal() {
    ttyfd = open("/dev/tty", O_RDWR);
    if (ttyfd < 0) {
        throw std::runtime_error("cmdrunner needs a terminal to show the menu");
    }

    if (tcgetattr(ttyfd, &saved) != 0) {
        ::close(ttyfd);
        ttyfd = -1;
        throw std::runtime_error("cmdrunner could not read the terminal state");
    }

    termios raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(ttyfd, TCSANOW, &raw) != 0) {
        ::close(ttyfd);
        ttyfd = -1;
        throw std::runtime_error("cmdrunner could not switch the terminal to raw mode");
    }
    rawset = true;
}

terminal::~terminal() {
    if (ttyfd >= 0) {
        if (rawset) {
            tcsetattr(ttyfd, TCSANOW, &saved);
        }
        ::close(ttyfd);
    }
}

int terminal::rows() const {
    winsize ws{};
    if (ioctl(ttyfd, TIOCGWINSZ, &ws) != 0 || ws.ws_row == 0) {
        return 24;
    }
    return ws.ws_row;
}

int terminal::cols() const {
    winsize ws{};
    if (ioctl(ttyfd, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) {
        return 80;
    }
    return ws.ws_col;
}
