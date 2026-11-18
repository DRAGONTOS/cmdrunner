# cmdrunner

A dmenu/rofi-compatible command runner inside the terminal. It reads choices
from stdin, shows a filterable menu and prints the picked line to stdout —
so it is a drop-in replacement for `rofi -dmenu` and dmenu scripts.

# Compatibility

- **Bare run**: nothing piped into stdin → lists every command in `$PATH` and
  executes the picked one, like `dmenu_run`.
- **Piped run** (dmenu contract): choices one per line from stdin, the
  selection printed to stdout, exit 0 on a pick and 1 on Escape.
- rofi/dmenu flags are accepted and ignored (`-dmenu`, `-show-icons`,
  `-theme <file>`, `-l <n>`, colors, fonts...); `-p <prompt>` sets the prompt.
- Anything that pipes choices into `rofi -dmenu` works with cmdrunner.

# Requirements

None beyond a compiler — no external dependencies.

# Installing

From source:
```
make all
sudo make install
```

Using rpm/deb:
```
make all
cd build
cpack -G RPM   # or -G deb
```

Using PKGBUILD:
```
makepkg -si PKGBUILD
```
