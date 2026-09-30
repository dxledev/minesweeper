# Minesweeper

A portable, standalone C++20 / Qt 6 game with a flat, modern interface, a resizable native window, and persistent live themes. Runs on Wayland and other platforms supported by Qt. Theme it directly from the CLI or follow a palette file; no theme manager is required.

## Build and run

Requires CMake 3.21+, a C++20 compiler, and Qt 6.7+ Core, Gui, and Widgets. Wayland also needs Qt's Wayland platform plugin. Tests require Qt Test and Bash. On Arch Linux, the build packages are `cmake`, `ninja`, `gcc`, `qt6-base`, and `qt6-wayland`.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
./build/minesweeper
```

Choose a difficulty in the window or start with `./build/minesweeper --difficulty expert`. Use `-DBUILD_TESTING=OFF` when configuring if Qt Test is unavailable. CMake also supports other build generators.

| Difficulty | Board | Mines |
| --- | --- | --- |
| Easy | 9 × 9 | 10 |
| Intermediate | 16 × 16 | 40 |
| Expert | 30 × 16 | 99 |

Mines are placed on the first reveal, with that cell and its neighbors protected. Empty areas open automatically. Clear every safe cell to win; flags help keep track of suspected mines. Boards are random and can require guessing.

The window can be tiled, floated, maximized, and resized. Cells scale to the available space and stay square. The minimum requested size is 580 × 680 logical pixels. A compositor may constrain a tiled window further. Wayland is selected automatically when `WAYLAND_DISPLAY` is present; `QT_QPA_PLATFORM` can override the choice.

## Controls

| Control | Action |
| --- | --- |
| Left click | Reveal a cell |
| Right click / Shift + left click | Toggle a flag |
| Double click a number / middle click | Reveal surrounding cells when adjacent flag count matches |
| Arrow keys / H J K L | Move the selected cell |
| Space | Reveal the selected cell |
| F | Toggle its flag |
| Enter | Reveal surrounding cells when adjacent flag count matches |
| P | Pause / resume |
| Ctrl + N | New game at the current difficulty |
| Escape | Dismiss the current modal |

**Flag mode** makes left click and Space place flags. Chording checks the flag count, not whether the flags are correct; incorrect flags can uncover a mine.

The timer begins on the first reveal and stops on a win or loss. Pausing hides the board and stops the timer. Losing window focus automatically pauses an active game; returning resumes an automatic pause. A manual pause stays paused.

Replacing an active game asks for confirmation in a modal inside the window. The timer pauses while the modal is open. Choose **Keep playing** or press **Escape** to return, or **New game** to confirm. The modal follows live theme changes and stays inside the window when it resizes. Game progress is held in memory for the current session; colors persist across launches.

**Stats**, next to **New game**, opens an in-window record for each difficulty: wins, losses, quits, fastest win, best win streak, and safe cells cleared. Stats pause the timer while open. A quit counts when you replace or close an unfinished game after at least **3:00 on its timer** and at least one move. Short attempts, untouched boards, and paused time do not count. Wins and losses are recorded immediately; streaks and cleared-cell totals include recorded games only.

Records are saved atomically to `stats.json` beside `theme.json`, persist across launches, and combine results from multiple windows. The same config-directory overrides apply. Invalid records are preserved and reported inline rather than overwritten. Game progress itself is not saved; closing a window ends that attempt.

## Live CLI themes

Keep the game open and run these commands in another terminal:

```bash
minesweeper theme list
minesweeper theme preset forest
minesweeper theme preset paper
minesweeper theme set 'accent=#c4b5fd' 'selection=#41344f'
minesweeper theme show
minesweeper theme path
```

Use `./build/minesweeper` in place of `minesweeper` before installing. Built-in presets are `forest`, `paper`, `slate`, and `rose`.

Each change validates the palette and atomically writes `theme.json`. Open windows watch that file and reload their colors without resetting the game or timer. Multiple windows sharing the same config directory receive updates. The CLI runs without a graphical session.

Colors live in `$XDG_CONFIG_HOME/minesweeper/theme.json`, falling back to `~/.config/minesweeper/theme.json`. Override the directory with `--config-dir DIRECTORY` or `MINESWEEPER_CONFIG_DIR`; use the same directory for the CLI and game.

```bash
minesweeper theme preset rose --dry-run
minesweeper theme export /tmp/my-theme.json
minesweeper theme import /tmp/my-theme.json
minesweeper --config-dir /tmp/minesweeper-preview theme preset slate
minesweeper --config-dir /tmp/minesweeper-preview
```

`--dry-run` validates and prints the proposed JSON without writing files. `theme list`, `theme path`, and `theme show` are read-only. Preset, set, and import commands stop following an external source. Export preserves any source binding.

See [themes/forest.json](themes/forest.json) for a complete palette. Colors use opaque `#RRGGBB` values; every role is configurable with `theme set`:

| Role | Use |
| --- | --- |
| `background` | Window and modal backdrop |
| `surface` | Board card, metric cards, and modal card |
| `surface_alt` | Covered cells and ordinary buttons |
| `text` | Main text and higher numbers |
| `muted` | Secondary text and hints |
| `accent` | Low numbers, flags, focus indicators, and primary buttons |
| `accent_text` | Text on primary buttons |
| `border` | Card and control outlines |
| `grid` | Revealed-cell outlines |
| `selection` | Hovered cells and buttons |
| `related` | Revealed-cell background |
| `matching` | Flagged-cell and selected-control backgrounds |
| `danger` | Mines, incorrect flags, loss messages, and theme errors |
| `success` | Completion message |

Direct JSON edits also reload live, including editors that replace the file atomically. Invalid updates keep the last valid colors and show an inline error. Fix the source to recover. An invalid saved theme at startup produces an error without overwriting the file. `theme preset NAME` can explicitly replace an invalid saved theme.

### Follow a palette

```bash
minesweeper theme follow file /absolute/path/to/palette.json
minesweeper theme follow file /absolute/path/to/theme-directory
```

The source binding persists. Minesweeper reads the source, saves the resolved colors to its own `theme.json`, and updates the running UI. It refreshes on startup and watches atomic source replacements, symlink changes, and replaced parent directories. Missing or invalid sources leave the last saved colors available. The source is only read and must differ from Minesweeper's output file.

Sources may use the native JSON schema, JSON objects with `background` / `text` / `accent` or common palette roles such as `base` / `text` / `primary`, or QML `property color` declarations. JSON `colors` and `colours` wrappers are supported, along with `dark` / `light` objects selected by a `mode` field. QML values may be quoted hex colors or numeric `Qt.rgba(...)` expressions. Expressions are parsed as data and never executed. Missing secondary roles are derived from the core palette.

For a directory, Minesweeper looks for `theme.json`, `palette.json`, `Colors.qml`, then `colors.json`, in that order. To follow a changing directory symlink, keep the logical symlink path in the command instead of resolving it first. `theme import FILE` takes a single snapshot instead of following future updates.

## Install

For a standard user installation:

```bash
cmake --install build --prefix "$HOME/.local"
```

Keep `~/.local/bin` on your `PATH`. This installs the executable, application entry named **Minesweeper**, and scalable icon in standard XDG locations.

An optional installer also stores the desktop entry in `~/.local/bin/applications`, registers it in `~/.local/share/applications`, and appends Minesweeper to `~/.config/apps.list`, preserving existing rows:

```bash
./scripts/install-desktop.sh --dry-run
./scripts/install-desktop.sh
```

Supply `--palette /absolute/path/to/palette` to establish a persistent source binding during installation. Use `--prefix`, `--build-dir`, `--config-dir`, or `--apps-list` for alternate absolute paths. Repeated installs avoid duplicate app-list entries. Changed config and app-list files receive timestamped backups. A normal reinstall without `--palette` preserves the saved theme.

## Validation and resource use

```bash
ctest --test-dir build --output-on-failure
QT_QPA_PLATFORM=wayland QT_WAYLAND_CLIENT_BUFFER_INTEGRATION=shm ./build/ui_tests
```

Tests cover mine counts, safe opening regions, number correctness, flags, flood reveals, wins and losses, correct and incorrect chording, keyboard and mouse input, compact layouts, pause timing, in-window modal behavior, theme persistence, invalid update recovery, source switches, atomic file and directory replacements, CLI dry runs, and installation preservation.

The board is one custom-painted widget rather than hundreds of cell widgets. Its largest game has 480 compact cells. Painting is event-driven, the one-second timer runs only during active play, and palette watching uses filesystem events instead of polling. Wayland uses raster/shared-memory buffers by default; set `QT_WAYLAND_CLIENT_BUFFER_INTEGRATION` to override this. No gradients, web view, persistent worker process, or GPU rendering loop are used.
