#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

// the user cancelled (Esc / Ctrl-C / Ctrl-D)
struct cancelled {};

// shows the dmenu-style picker on /dev/tty and returns the picked line.
// reverse owns the whole terminal window with the prompt on top (fzf
// --layout=reverse style); the default layout renders inline at the cursor.
std::string pick(const std::vector<std::string> &items, const std::string &prompt, bool reverse);

#endif
