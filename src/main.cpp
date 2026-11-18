#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>
#include "includes/menu.hpp"

namespace {

// every executable found in the PATH dirs, deduped by name and sorted — the dmenu_run list
std::vector<std::string> pathcommands() {
    const char *pathenv = getenv("PATH");
    if (!pathenv) {
        return {};
    }

    std::vector<std::string> out;
    std::filesystem::path dir;
    std::set<std::string> seen;
    const char *start = pathenv;
    for (const char *p = pathenv;; ++p) {
        if (*p != ':' && *p != '\0') {
            continue;
        }
        dir.assign(start, p);
        start = p + 1;
        if (dir.empty()) {
            continue;
        }

        std::error_code ec;
        std::filesystem::directory_iterator it(dir, ec), end;
        while (!ec && it != end) {
            const std::filesystem::path file = it->path();
            it.increment(ec);
            std::error_code fec;
            if (!std::filesystem::is_regular_file(file, fec) || fec) {
                continue;
            }
            if (::access(file.c_str(), X_OK) != 0) {
                continue;
            }
            const std::string name = file.filename().string();
            if (seen.insert(name).second) {
                out.push_back(name);
            }
        }
        if (*p == '\0') {
            break;
        }
    }

    std::sort(out.begin(), out.end());
    return out;
}

std::string shellquote(const std::string &value) {
    std::string quoted = "'";
    for (const char c : value) {
        if (c == '\'') {
            quoted += "'\\''";
        } else {
            quoted += c;
        }
    }
    quoted += "'";
    return quoted;
}

} // namespace

int main(int argc, char **argv) {
    std::string prompt;
    bool fullscreen = true;

    // rofi/dmenu compatibility: -p takes the prompt, flags known to take a
    // value are skipped together with it, every other flag is ignored.
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-p" || arg == "--prompt") {
            if (i + 1 < argc) {
                prompt = argv[++i];
            }
            continue;
        }
        if (arg == "-fullscreen" || arg == "--fullscreen") {
            fullscreen = true;
            continue;
        }
        if (arg == "-no-fullscreen" || arg == "--no-fullscreen") {
            fullscreen = false;
            continue;
        }

        for (const std::string takesvalue : {"-theme", "-l", "-m", "-fn", "-nb", "-nf", "-sb", "-sf", "-config"}) {
            if (arg == takesvalue || arg == "-" + takesvalue) {
                if (i + 1 < argc) {
                    ++i;
                }
                break;
            }
        }
    }

    std::vector<std::string> items;
    // bare run with nothing piped in: list the PATH commands like dmenu_run does
    const bool launcher = isatty(0);
    if (launcher) {
        items = pathcommands();
        if (items.empty()) {
            std::cerr << "cmdrunner: no commands found in PATH\n";
            return 1;
        }
    } else {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty()) {
                items.push_back(line);
            }
        }
    }

    if (items.empty()) {
        return 1;
    }

    try {
        const std::string choice = pick(items, prompt, fullscreen);
        if (launcher) {
            const int rc = system(("sh -c " + shellquote(choice)).c_str());
            if (rc != 0) {
                std::cerr << "cmdrunner: the command failed: " << choice << "\n";
            }
            return 0;
        }
        std::cout << choice << "\n";
        return 0;
    } catch (const cancelled &) {
        return 1;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
