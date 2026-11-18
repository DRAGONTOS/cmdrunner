# Agent guide

cmdrunner is a dmenu/rofi-compatible command runner inside the terminal: it reads choices one per line from stdin, shows a filterable menu on /dev/tty and prints the picked line to stdout — a drop-in replacement for `rofi -dmenu`/dmenu scripts, so rapidmenu works with it unchanged. Interaction is fzf-styled (type to filter, arrow keys, Enter, Esc; matched text bold, pointer + info line, prompt at the bottom). Written in C++26 on CMake and Ninja, no external dependencies.

Cross-session handoff notes for this project live in `~/.local/share/agents/cmdrunner-handoff.md`. Read that file first when resuming after a context reset, and update it when state changes (commits, queue changes, new gotchas).

Sibling repos with the same conventions: `~/dev/programming/rapidmenu` and `~/dev/programming/stc`.

## Architecture

| Part | Where | Notes |
|---|---|---|
| Entry and args | `src/main.cpp` | Reads stdin lines (the choices) when stdin is piped; **bare run lists the `$PATH` executables** (deduped, sorted) and executes the picked command via `sh -c` — `dmenu_run` behavior. Piped run keeps the dmenu contract: pick printed to stdout, exit 0 on pick, 1 on cancel/empty stdin/no terminal. Parses dmenu/rofi flags: `-p`/`--prompt` sets the prompt; `--layout=reverse` (the **default**) and `--layout=default` switch the layout — fzf-style names, the space form `--layout reverse` works too; a known value-taking flag (`-theme`, `-l`, `-m`, `-fn`, `-nb`, `-nf`, `-sb`, `-sf`, `-config`) skips its value; every other flag (`-dmenu`, `-show-icons`, ...) is ignored. |
| Menu | `src/menu.cpp` | `pick` is the whole interaction: case-insensitive substring filter, a window over the filtered list sized to the terminal, **fzf-style rendering copied from a captured fzf run** — a `▌` gutter bar on every row (purple 256-color 141 on the selected row over a full-line dark bar `48;5;236`, dim gray `38;5;236` elsewhere), selected text bold near-white (`38;5;254`) with the matched substring flipped to purple, a dim info line (`n/total` in purple + a `─` box-drawing rule filling the width in `38;5;60`), and the `<prompt> >` line in purple bold with the cursor blinking after the query. Both layouts render **under the shell prompt, never taking the screen** — the menu starts one line below the cursor and only ever erases its own lines. `--layout=reverse` (the default) puts the search and info line on top with the items below; `--layout=default` puts the search at the bottom. Keys: Enter picks, plain Esc / Ctrl-C / Ctrl-D cancel (`cancelled` exception), arrows move (the only navigation — letter keys all type into the query), backspace and Ctrl-U edit the query. Escape sequences that arrive split across reads are completed with a VTIME peek read. |
| Terminal | `src/terminal.cpp` | RAII over `/dev/tty`: open read+write, raw mode (no echo/canonical/signals), saved termios restored on destruction; `rows`/`cols` via `TIOCGWINSZ` with 24x80 fallback. |
| Version | `props.json` | Single `"version"` key, read by CMake at configure time and by the PKGBUILD's `pkgver()`. Bump it for every release, and update the `line-count` file (cloc over `src/`) in the same commit. |

## Build and install

```sh
make all
sudo make install
```

`make all` clears `build/` and configures with Ninja, building Release with `-O2`. Packaging:

```sh
cd build && cpack -G DEB   # or RPM
makepkg -si PKGBUILD       # Arch
```

`.github/workflows/builds.yml` builds master and tags on ubuntu-24.04 with g++-14 and publishes the binary, a deb and an rpm to the releases: a rolling `latest` prerelease replaced on every master push, and a pinned release per `v*` tag whose notes list the commits since the previous tag. **Every tagged release shows what changed** — tag after the fixes land (move with `git tag -f` + force-push if needed), never with stale notes or a stale tag. The workflow's rolling release also creates/moves a `latest` git tag on the remote every master push — don't confuse it with a version tag.

## Hard rules

- Never push (git or a release) without the owner's explicit approval.
- Never run `sudo` or `make install` on your own; building and running the binary from `build/` is fine.
- GPLv3, keep the LICENSE.
- Commit subjects are lowercase with a type prefix and no body: `fix:`, `added:`, `bump:`, `cleanup:`, `major:`, `pkgbuild:`. Version bumps: `Bump: bumped to (x.y.z)`.
- The version in `props.json` is the single source of truth; bump it and `line-count` together.
- **Every tagged release shows what changed**: the builds workflow generates the release notes from the commits since the previous tag.
- Don't add a comment where the code can say it itself; the codebase does comment, but sparingly — follow the surrounding file.

## Before pushing

- `make all` builds clean.
- A `-p <prompt>` run with rofi-style flags picked an item (or cancelled) — verify the compatibility contract: stdin in, pick out, exit 0 on pick / 1 on cancel.
- `props.json`, `line-count` and any packaging metadata agree on the version.

## Code style

- C++26 (`set(CMAKE_CXX_STANDARD 26)`), target `cmdrunner`, sources globbed from `src/` recursively — a new file needs no CMake edit.
- Indentation is a mix of 2 and 4 spaces across files; match the file you are editing rather than reformatting.
- Errors: plain `exit(1)` with stderr prints, `throw` for fatal setup failures (caught in `main.cpp`); keep new errors consistent with their file.
- No test suite exists. Verification is manual: `socat` can fake the pty — `(sleep 1; printf '\r') | socat - 'EXEC:<script that pipes items into cmdrunner>,pty,ctty,setsid,raw,echo=0'`.

## Gotchas

- **stdin carries the choices, /dev/tty carries the keys and the drawing** — stdin is a pipe in the rapidmenu flow (already consumed), so keyboard input must never be read from stdin. The mode split is `isatty(0)`: tty stdin → PATH launcher that executes; piped stdin → print-only dmenu contract.
- The selection goes to stdout, the menu UI to /dev/tty — that split is what lets rapidmenu's `popen` capture the pick while the user sees the menu.
- When stdout is piped (rapidmenu's flow) everything still works: the menu draws on /dev/tty regardless.
- `/dev/tty` missing (no controlling terminal) exits 1 with a message — never abort.
- A new CMake project needs the full CPack block (`include(CPack)`, vendor/maintainer, `CPACK_GENERATOR`) or `cpack -G DEB` fails in CI with `CPack project name not specified` — this cost the first cmdrunner CI run.
- The `latest` git tag on the remote is created/moved by the rolling release, not a version tag — don't confuse them.
- ubuntu-24.04's default g++ cannot take `-std=c++26` — the workflow pins g++-14.
