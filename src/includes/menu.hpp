#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

// the user cancelled (Esc / Ctrl-C / Ctrl-D)
struct cancelled {};

// shows the dmenu-style picker on /dev/tty and returns the picked line.
// reverse puts the search on top and draws right under the shell prompt;
// the default layout renders with the search at the bottom.
std::string pick(const std::vector<std::string> &items, const std::string &prompt, bool reverse);

#endif
