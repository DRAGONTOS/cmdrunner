#include "includes/menu.hpp"
#include "includes/terminal.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

std::string lower(const std::string &s) {
    std::string out;
    for (const char c : s) {
        out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

void writeall(int fd, const std::string &s) {
    size_t off = 0;
    while (off < s.size()) {
        ssize_t n = ::write(fd, s.data() + off, s.size() - off);
        if (n <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        off += static_cast<size_t>(n);
    }
}

// one line, truncated to the terminal width
std::string visible(const std::string &s, int width) {
    if (static_cast<int>(s.size()) <= width) {
        return s;
    }
    return s.substr(0, static_cast<size_t>(width));
}

} // namespace

std::string pick(const std::vector<std::string> &items, const std::string &prompt, const bool reverse) {
    terminal tty;
    const int fd = tty.fd();

    std::string query;
    std::string result;
    bool done = false;
    bool cancelledrun = false;
    int drawn = 0; // inline layout only: lines the last frame occupied

    std::vector<size_t> filtered;
    size_t sel = 0;
    size_t offset = 0;

    auto refilter = [&]() {
        filtered.clear();
        const std::string q = lower(query);
        for (size_t i = 0; i < items.size(); ++i) {
            if (q.empty() || lower(items[i]).find(q) != std::string::npos) {
                filtered.push_back(i);
            }
        }
        if (filtered.empty() || sel >= filtered.size()) {
            sel = 0;
        }
        offset = 0;
    };

    auto height = [&]() {
        int h = tty.rows() - 2; // the prompt line + the info line + the item lines
        return h < 1 ? 1 : h;
    };

    // the matched part of the text in the accent color, fzf style; open/close
    // bring back the surrounding line colors without touching the background
    auto highlight = [&](const std::string &text, const std::string &open, const std::string &close) {
        if (query.empty()) {
            return text;
        }
        const size_t pos = lower(text).find(lower(query));
        if (pos == std::string::npos) {
            return text;
        }
        return text.substr(0, pos) + open + text.substr(pos, query.size()) + close +
               text.substr(pos + query.size());
    };

    // cmdrunner draws right under the shell prompt: --layout=reverse puts the
    // search on top, --layout=default at the bottom. only the menu's own lines
    // are touched, never the rest of the screen.
    auto draw = [&]() {
        std::string frame;
        if (drawn == 0) {
            frame += "\n"; // the menu starts one line below the prompt
        } else {
            frame += "\r\x1b[" + std::to_string(drawn - 1) + "A\x1b[J";
        }
        frame += "\x1b[?25l";

        const int h = height();
        if (sel < offset) {
            offset = sel;
        }
        if (sel >= offset + static_cast<size_t>(h)) {
            offset = sel - static_cast<size_t>(h) + 1;
        }

        const int width = tty.cols() - 1;
        const size_t end = std::min(filtered.size(), offset + static_cast<size_t>(h));

        auto itemline = [&](size_t i) {
            const std::string text = visible(items[filtered[i]], width - 2);
            if (i == sel) {
                return "\x1b[1;38;5;141;48;5;236m\xe2\x96\x8c\x1b[48;5;236m \x1b[1;38;5;254m" +
                       highlight(text, "\x1b[38;5;141m", "\x1b[22;38;5;254m") + "\x1b[K\x1b[0m";
            }
            return "\x1b[38;5;236m\xe2\x96\x8c\x1b[0m " +
                   highlight(text, "\x1b[1;38;5;141m", "\x1b[22;39m") + "\x1b[K";
        };

        const std::string count = std::to_string(filtered.size()) + "/" + std::to_string(items.size());
        std::string rule;
        for (int i = 0; i < tty.cols() - 3 - static_cast<int>(count.size()); ++i) {
            rule += "\xe2\x94\x80";
        }
        const std::string inforow = "  \x1b[38;5;141m" + count + "\x1b[38;5;60m " + rule + "\x1b[0m\x1b[K";

        const std::string pfx = prompt.empty() ? "> " : (prompt.back() == ' ' ? prompt + "> " : prompt + " >");
        const std::string promptrow = "\x1b[1;38;5;141m" + pfx + "\x1b[0m\x1b[1m" + query + "\x1b[K";

        if (reverse) {
            // the search and the info line on top, the items below
            frame += promptrow + "\n" + inforow;
            for (size_t i = offset; i < end; ++i) {
                frame += "\n" + itemline(i);
            }
        } else {
            // the items, then the info line, then the search at the bottom
            for (size_t i = offset; i < end; ++i) {
                frame += "\n" + itemline(i);
            }
            frame += "\n" + inforow + "\n" + promptrow;
        }

        drawn = 2 + static_cast<int>(end - offset);
        writeall(fd, frame);
    };

    auto cleanup = [&]() {
        std::string out;
        if (drawn > 0) {
            out = "\r\x1b[" + std::to_string(drawn - 1) + "A\x1b[J";
        }
        out += "\x1b[?25h";
        writeall(fd, out);
    };

    refilter();
    draw();

    // a VTIME-bounded read to complete escape sequences split across reads
    auto peekread = [&](unsigned char *out, size_t size) -> ssize_t {
        termios peek{};
        tcgetattr(fd, &peek);
        peek.c_cc[VMIN] = 0;
        peek.c_cc[VTIME] = 1;
        tcsetattr(fd, TCSANOW, &peek);
        ssize_t m = ::read(fd, out, size);
        peek.c_cc[VMIN] = 1;
        peek.c_cc[VTIME] = 0;
        tcsetattr(fd, TCSANOW, &peek);
        return m;
    };

    while (!done) {
        unsigned char buf[32];
        ssize_t n;
        do {
            n = ::read(fd, buf, sizeof(buf));
        } while (n < 0 && errno == EINTR);
        if (n <= 0) {
            cancelledrun = true;
            break;
        }

        // an Esc or Esc + [ at the end of the buffer may continue in the next read
        while (n > 0 && (buf[n - 1] == 27 || (n >= 2 && buf[n - 2] == 27 && buf[n - 1] == '['))) {
            unsigned char more[8];
            const ssize_t m = peekread(more, sizeof(more));
            if (m <= 0) {
                break;
            }
            if (static_cast<size_t>(n) + static_cast<size_t>(m) > sizeof(buf)) {
                break;
            }
            std::memcpy(buf + n, more, static_cast<size_t>(m));
            n += m;
        }

        for (ssize_t i = 0; i < n; ++i) {
            const unsigned char c = buf[i];
            if (c == '\n' || c == '\r') {
                if (!filtered.empty()) {
                    result = items[filtered[sel]];
                }
                done = true;
                break;
            }
            if (c == 27) {
                if (i + 1 < n && buf[i + 1] == '[' && i + 2 < n) {
                    if (buf[i + 2] == 'A') {
                        if (sel > 0) {
                            --sel;
                        }
                    } else if (buf[i + 2] == 'B') {
                        if (sel + 1 < filtered.size()) {
                            ++sel;
                        }
                    }
                    i += 2;
                    continue;
                }
                // plain Esc (no sequence): cancel
                cancelledrun = true;
                done = true;
                break;
            }
            if (c == 127 || c == 8) { // backspace
                if (!query.empty()) {
                    query.pop_back();
                    refilter();
                }
                continue;
            }
            if (c == 21) { // Ctrl-U
                query.clear();
                refilter();
                continue;
            }
            if (c == 3 || c == 4) { // Ctrl-C, Ctrl-D
                cancelledrun = true;
                done = true;
                break;
            }
            if (c >= 32 && c != 127) {
                query += static_cast<char>(c);
                refilter();
            }
        }

        if (!done) {
            draw();
        }
    }

    cleanup();

    if (cancelledrun || result.empty()) {
        throw cancelled{};
    }
    return result;
}
