# cmdrunner

A dmenu/rofi-compatible command runner inside the terminal. It reads choices
from stdin, shows a filterable menu and prints the picked line to stdout —
so it is a drop-in replacement for `rofi -dmenu` and dmenu scripts.

# Compatibility

- **Bare run**: nothing piped into stdin → lists every command in `$PATH` and
  executes the picked one, like `dmenu_run`.
- **Piped run** (dmenu contract): choices one per line from stdin, the
  selection printed to stdout, exit 0 on a pick and 1 on Escape.
- The menu draws **right under your shell prompt** — the search on top by
  default (`--layout=reverse`), or the search at the bottom with
  `--layout=default`. It never takes the screen; only its own lines are
  redrawn and erased.
- rofi/dmenu flags are accepted and ignored (`-dmenu`, `-show-icons`,
  `-theme <file>`, `-l <n>`, colors, fonts...); `-p <prompt>` sets the prompt.
- Anything that pipes choices into `rofi -dmenu` works with cmdrunner.

# Requirements

None beyond a compiler — no external dependencies.

# Configuration

`~/.config/cmdrunner/config.toml` is read when it exists:

```toml
layout = "reverse"   # "reverse" (search on top, the default) or "default" (search at the bottom)
prompt = ""          # shown in front of the query
```

Flags override the config (`-p`, `--layout`). A missing file is fine; a
broken one warns and keeps the defaults.

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
