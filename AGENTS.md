# Agent guide

cmdrunner is a brand-new project — currently **only the scaffold exists**: CMake + Ninja build, packaging (deb/rpm/PKGBUILD), a stc-style builds workflow and a placeholder `main`. **The feature set is not pinned down yet** — before implementing anything, ask the owner what cmdrunner is supposed to do. The `project` DESCRIPTION ("Run commands."), the PKGBUILD `pkgdesc` and the README are placeholders until then.

Cross-session handoff notes for this project live in `~/.local/share/agents/cmdrunner-handoff.md`. Read that file first when resuming after a context reset, and update it when state changes (commits, queue changes, new gotchas).

Sibling repos with the same conventions: `~/dev/programming/rapidmenu` and `~/dev/programming/stc`.

## Architecture

| Part | Where | Notes |
|---|---|---|
| Entry | `src/main.cpp` | Placeholder — prints a notice and exits 0. Sources are globbed from `src/` recursively, so new files need no CMake edit. |
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
- `USAGE`/`HELP` strings (once they exist) match the flags actually implemented.
- `props.json`, `line-count` and any packaging metadata agree on the version.

## Code style

- C++26 (`set(CMAKE_CXX_STANDARD 26)`), target `cmdrunner`, sources globbed from `src/` recursively.
- Indentation is a mix of 2 and 4 spaces in the sibling repos; match the file you are editing rather than reformatting.
- Errors: plain `exit(1)` with stderr prints unless a better pattern establishes itself; keep new errors consistent with their file.
- No test suite exists. Verification is manual.
