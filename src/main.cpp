#include <iostream>
#include <string>
#include <vector>
#include "includes/menu.hpp"

int main(int argc, char **argv) {    std::string prompt = "menu";

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
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty()) {
            items.push_back(line);
        }
    }

    if (items.empty()) {
        return 1;
    }

    try {
        std::cout << pick(items, prompt) << "\n";
        return 0;
    } catch (const cancelled &) {
        return 1;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
