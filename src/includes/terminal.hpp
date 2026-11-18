#ifndef TERMINAL_H
#define TERMINAL_H

#include <termios.h>

// owns /dev/tty: raw mode for the keyboard, the fd for drawing.
// keys come from /dev/tty because stdin carries the piped-in choices.
class terminal {
public:
    terminal();
    ~terminal();

    terminal(const terminal &) = delete;
    terminal &operator=(const terminal &) = delete;

    int fd() const { return ttyfd; }
    int rows() const;
    int cols() const;

private:
    int ttyfd = -1;
    bool rawset = false;
    termios saved{};
};

#endif
