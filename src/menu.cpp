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

std::string pick(const std::vector<std::string> &items, const std::string &prompt) {
    terminal tty;
    const int fd = tty.fd();

    std::string query;
    std::string result;
    bool done = false;
    bool cancelledrun = false;
    int drawn = 0; // lines the last frame occupied; the cursor rests at the end of the last one

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
        int h = tty.rows() - 2; // item lines + the info line + the prompt line
        return h < 1 ? 1 : h;
    };

    // the matched part of the text in bold, fzf style
    auto highlight = [&](const std::string &text) {
        if (query.empty()) {
            return text;
        }
        const size_t pos = lower(text).find(lower(query));
        if (pos == std::string::npos) {
            return text;
        }
        return text.substr(0, pos) + "\x1b[1m" + text.substr(pos, query.size()) + "\x1b[0m" +
               text.substr(pos + query.size());
    };

    auto draw = [&]() {
        std::string frame;
        if (drawn > 0) {
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
        for (size_t i = offset; i < end; ++i) {
            frame += "\n";
            const std::string text = highlight(visible(items[filtered[i]], width - 2));
            if (i == sel) {
                frame += "\x1b[1;36m>\x1b[0m " + text + "\x1b[K";
            } else {
                frame += "  " + text + "\x1b[K";
            }
        }

        // the info line and the prompt live at the bottom, fzf style
        frame += "\n\x1b[2m  " + std::to_string(filtered.size()) + "/" + std::to_string(items.size()) + "\x1b[0m\x1b[K";
        frame += "\n\x1b[1;36m" + (prompt.empty() ? ">" : prompt + " >") + "\x1b[0m " + query + "\x1b[K\x1b[?25h";

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
