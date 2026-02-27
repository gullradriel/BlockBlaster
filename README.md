<img width="298" height="447" alt="image" src="https://github.com/user-attachments/assets/58d6f3da-0913-4607-93d4-6628ca000d9e" /> <img width="302" height="452" alt="image" src="https://github.com/user-attachments/assets/4f9c65ae-67fa-43d1-b809-8fc022f7199a" />

# Block Blaster

A fast-paced block-placement puzzle game inspired by Block Blast. Drag and drop pieces onto the grid to clear rows and columns -- simple to learn, endlessly satisfying to master.

Download and compile it yourself, or [play the online, browser version](https://www.nilorea.net/Games/BlockBlaster/)

Repository contain a Linux binary (compiled on Debian testing, need [some additional libraries](https://github.com/gullradriel/BlockBlaster?tab=readme-ov-file#linux-dependencies)) to work)

Android version is only available as a DEBUG APK, until I get it to the play store

Built with [Allegro 5](https://liballeg.org/) in C99. Runs on Linux, Windows (MinGW), WebAssembly (Emscripten) and Android.

## Gameplay

- You are given a **configurable grid** (10x10, 15x15 or 20x20) and a set of **1 to 4 block pieces** each round.
- **Drag pieces** from the tray onto the board to place them.
- When a **full row or column** is filled, it clears and you earn points.
- Clearing **multiple lines at once** scores bonus points -- combos are key.
- The game ends when **no remaining piece can fit** on the board.

Two starting modes are available from the main menu:

| Mode | Description |
|---|---|
| Empty grid | Start with a clean board |
| Partially filled grid | Start with 16-28 random cells pre-filled for added challenge |

## Features

- Configurable grid sizes: **10x10**, **15x15**, **20x20**
- Configurable tray size: **1 to 4 pieces** per round
- Smooth drag-and-drop controls with ghost preview and snap-to-grid
- Predicted-clear highlighting shows which rows/columns will clear before you drop
- Row and column clearing with **combo multipliers** (up to x20)
- **Difficulty ramp**: shapes get harder as your score increases (weighted bag randomizer)
- **Top-5 high-score table** with player names, tracked per grid/tray configuration
- Particle burst effects on line clears with screen shake
- Animated "+N" score popups and centred "COMBO xN" popup
- Return-to-tray animation on invalid drops
- Pop-scale animation when pieces are placed
- Sound effects (place, select, return, line-clear) and background music tracks
- Toggleable sound from the menu or in-game
- Fullscreen toggle (F11 on desktop, browser Fullscreen API on web)
- Persistent settings: sound state, grid size, tray count, player name, high scores
- On Emscripten saves are persisted via IDBFS (IndexedDB)
- Android display density scaling for readable text on high-DPI screens
- Tab visibility handling on web (pauses timer when tab is hidden)
- Resizable window with automatic UI scaling
- Exit confirmation dialog during gameplay

## Controls

| Input | Action |
|---|---|
| Mouse drag | Pick up a piece from the tray and drop it on the grid |
| Touch drag (Android) | Same as mouse drag; piece is offset upward to stay visible |
| F11 | Toggle fullscreen (desktop) |
| Escape | Open/close exit confirmation dialog (in-game) or quit (menu) |
| Letter keys | Type player name on game-over screen (A-Z, up to 5 characters) |
| Backspace | Delete last character of player name |
| Enter | Confirm player name |

## Architecture

The codebase is split into focused modules:

| File | Responsibility |
|---|---|
| `BlockBlaster.c` | Entry point, Allegro init, main event loop, cleanup |
| `blockblaster_game.c` | All game logic: grid ops, scoring, bag randomizer, save/load, animations |
| `blockblaster_render.c` | Drawing: grid, tray, ghost preview, particles, popups, floating piece |
| `blockblaster_ui.c` | Menu, buttons, hit-testing, game-over overlay, fullscreen toggle |
| `blockblaster_audio.c` | Audio loading, SFX playback, music track switching |
| `blockblaster_context.h` | All data structures, constants, and layout macros |
| `blockblaster_shapes.h` | Static table of 58 block shapes (ordered easy to hard) |
| `allegro_emscripten_mouse.c/.h` | Browser Pointer Lock integration (Emscripten only) |
| `allegro_emscripten_fullscreen.c/.h` | Fullscreen change callback, tab visibility, keyboard layout capture (Emscripten only) |

Utility libraries (bundled, not external dependencies):

| File | Description |
|---|---|
| `n_common.c` / `n_common.h` | Common macros and helpers (part of Nilorea library) |
| `n_log.c` / `n_log.h` | Logging utilities |
| `n_str.c` / `n_str.h` | String utilities |
| `n_list.c` / `n_list.h` | Linked list utilities |
| `cJSON.c` / `cJSON.h` | JSON parsing library (third-party) |

### Game state machine

```
STATE_MENU  -->  STATE_PLAY  -->  STATE_GAMEOVER  -->  STATE_MENU
                     ^                  |
                     |  (restart)       |
                     +------------------+
```

### Scoring system

| Event | Points |
|---|---|
| Each cell placed | 1 |
| Each cell cleared | 10 |
| Each full line cleared | 25 bonus |
| Each additional line (multi-line) | 50 bonus |
| Combo multiplier | +0.25 per consecutive clearing move (max x20) |

Three consecutive non-clearing moves reset the combo counter.

### Difficulty system

Shapes are ordered from easiest (1-cell single dot) to hardest (5x5 cross) in the `SHAPES[]` array. A weighted bag randomizer draws shapes biased toward easy shapes at low scores and hard shapes at high scores. Every shape always retains at least 1% probability. Maximum difficulty is reached at 75,000 points.

### Save data

All save files are plain text, stored per-platform:

| Platform | Save directory |
|---|---|
| Desktop | `./DATA/` |
| Emscripten | `/save/` (IDBFS, synced to IndexedDB) |
| Android | App internal storage (via JNI) |

Files persisted:

| File | Contents |
|---|---|
| `blockblaster_scores.txt` | Top-5 high scores with grid size, tray count, combo, and player name |
| `blockblaster_playername.txt` | Last-used player name |
| `blockblaster_sound_state.txt` | Sound on/off state |
| `blockblaster_settings.txt` | Tray count and grid size |

## DATA directory

Game assets bundled in the `DATA/` directory:

| File | Description |
|---|---|
| `game_sans_serif_7.ttf` | Game font (scaled dynamically based on display size) |
| `place.ogg` | SFX: piece placed on grid |
| `select.ogg` | SFX: piece selected from tray |
| `send_to_tray.ogg` | SFX: piece returns to tray (invalid drop) |
| `break_lines.ogg` | SFX: lines cleared |
| `intro.ogg` | Music: main menu / game over |
| `music1.ogg` | Music: in-game track 1 |
| `music2.ogg` | Music: in-game track 2 |

# Linux dependencies

```
sudo apt-get install -y \
  liballegro-acodec5.2 \
  liballegro-audio5.2 \
  liballegro-primitives5.2 \
  liballegro-ttf5.2 \
  liballegro-font5.2 \
  liballegro5.2 \
  libopenal1 \
  libdbus-1-3
```

## Contributing

Contributions are welcome! Feel free to open an issue or submit a pull request.

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m 'Add my feature'`
4. Push to the branch: `git push origin feature/my-feature`
5. Open a Pull Request

## License

This project is licensed under the GPLv3 License. See [LICENCE](LICENCE) for details.

## Acknowledgements

Inspired by [Block Blast](https://blockblastgame.net/). 

Built for fun and learning, not affiliated with the original.

I just didn't want my kids to stumble upon bad ads.

### Music and sounds from [Pixabay](https://pixabay.com/fr/sound-effects/search/) :

- [intro](https://pixabay.com/fr/sound-effects/musical-cooking-background-music-loop-486763/)
- [game music 1](https://pixabay.com/fr/sound-effects/musical-mechanical-music-machine-1-35448/)
- [game music 2](https://pixabay.com/fr/sound-effects/musical-8bit-music-for-game-68698/)
- [SFX pop](https://pixabay.com/fr/sound-effects/films-et-effets-sp%c3%a9ciaux-sharp-pop-328170/)
- [SFX click](https://pixabay.com/fr/sound-effects/films-et-effets-sp%c3%a9ciaux-click-sound-432501/)
- [SFX slam](https://pixabay.com/sound-effects/film-special-effects-metal-slam-3-189785/)
- [SFX hit](https://pixabay.com/sound-effects/film-special-effects-hit-by-a-wood-230542/)

### Font from [1001freefonts](https://www.1001freefonts.com)

[game-sans-serif-7.font](https://www.1001freefonts.com/game-sans-serif-7.font)

### Coding, build system, comments

I used [Claude](https://claude.ai/), particularly [Claude Code](https://claude.ai/code) for:
- bug checking / bug fixing
- Makefile upgrade to support Android
- Generate all code comments
- Generate that README (that I also edited manually)

---

# Build Documentation

> Supports **Linux**, **Windows (MinGW)**, **WebAssembly (Emscripten)**, and **Android**.

## Prerequisites

| Platform | Required tools |
|---|---|
| Linux | `gcc`, `make`, Allegro 5 dev libraries (`liballegro5-dev` or equivalent) |
| Windows | MinGW (MSYS2: `MINGW32`, `MINGW64`, or `MINGW64CB`) + Allegro 5 |
| WebAssembly | Emscripten SDK (`emsdk`), `cmake`, `make` |
| Android | Android SDK + NDK, `cmake`, `javac`, `keytool` |

### Allegro 5 libraries used

The game links against the following Allegro 5 add-on libraries:

- `allegro` (core)
- `allegro_font` + `allegro_ttf` (TrueType font rendering)
- `allegro_primitives` (line/rectangle/circle drawing)
- `allegro_audio` + `allegro_acodec` (sound effects and music)
- `allegro_image` (image loading)
- `allegro_color` (colour utilities)
- `allegro_main` (main() redirection on some platforms)

## Native Builds (Linux / Windows)

### `make` / `make all`
Builds the main `BlockBlaster` executable (or `BlockBlaster.exe` on Windows).

```sh
make
```

Compiler flags are auto-detected based on the OS and `MSYSTEM` environment variable:

| Environment | Notes |
|---|---|
| Linux | `gcc`, optimized with `-O3`, links Allegro 5 + pthreads |
| MINGW32 | 32-bit Windows build (`-m32`), links pthreads + WinSock |
| MINGW64 | 64-bit Windows build (`-DARCH64BITS`) |
| MINGW64CB | 64-bit Windows, uses `del /Q` for cleanup (Code::Blocks IDE) |
| SunOS | Uses `cc` with Solaris-specific flags |

### `make clean`
Removes compiled object files and the `BlockBlaster` binary.

```sh
make clean
```

## WebAssembly (Emscripten) Build

Produces `build_emscripten/BlockBlaster.html`, runs in the browser via WASM.

### Setup

The Makefile auto-detects the Emscripten SDK at `$(HOME)/emsdk` (the default
install location used by the official `emsdk` installer).  No manual environment
sourcing is required when the SDK is installed there.

If the SDK is installed elsewhere, either set `EMSDK_DIR` or `EMSDK` on the
command line, or source `emsdk_env.sh` beforehand:

```sh
# Option A -- override install directory
make wasm EMSDK_DIR=/opt/emsdk

# Option B -- let emsdk_env.sh set $EMSDK (still works as before)
source /path/to/emsdk/emsdk_env.sh
make wasm
```

### Configurable variables

| Variable | Default | Description |
|---|---|---|
| `EMSDK_DIR` | `~/emsdk` | Path to the Emscripten SDK install directory |
| `EMSDK` | `$(EMSDK_DIR)` | SDK root used to derive `emcc`/`emcmake`; honoured if already set in the environment |
| `EMCC` | `$(EMSDK)/upstream/emscripten/emcc` | Path to the `emcc` compiler |
| `EMCMAKE` | `$(EMSDK)/upstream/emscripten/emcmake` | Path to the `emcmake` wrapper |

### Targets

| Target | Description |
|---|---|
| `make wasm-deps` | Clones `libogg`, `libvorbis`, `freetype` and `allegro5` repos if missing |
| `make wasm-setup` | Verifies `emcc` is executable, creates build directories |
| `make wasm-libogg` | Builds `libogg` for WASM |
| `make wasm-libvorbis` | Builds `libvorbis` for WASM (depends on `wasm-libogg`) |
| `make wasm-allegro` | Builds Allegro 5 monolith for WASM (depends on `wasm-libvorbis`) |
| `make wasm` | Full WASM build, compiles `BlockBlaster.html` (depends on all above) |
| `make wasm-clean` | Removes all WASM build artifacts |

### Full WASM build (one command)

```sh
make wasm
```

### Running the WASM build

```sh
cd build_emscripten
python3 -m http.server
# Open http://localhost:8000/BlockBlaster.html
```

### Notable WASM flags

- `ASYNCIFY`: required because Allegro's `al_wait_for_event()` must yield to the browser
- `INITIAL_MEMORY=2147418112`: ~2 GB initial heap (accommodates large grid sizes)
- `FULL_ES3=1`: full OpenGL ES 3 support
- `FILESYSTEM=1`: virtual filesystem enabled (needed for IDBFS saves)
- `USE_SDL=2`: Emscripten's built-in SDL2 backend (required by Allegro's Emscripten port)
- Assets are bundled via `--preload-file DATA`
- Cache-busting: output files are renamed with a content hash to prevent stale browser caches

### Emscripten-specific behaviour

- **IDBFS saves**: on first load, a `/save/` directory is mounted as IDBFS and synced from IndexedDB. High scores, settings, and player name persist across browser sessions.
- **Tab visibility**: when the browser tab is hidden the game timer is paused and the event queue is flushed to prevent a burst of accumulated events on return.
- **Keyboard layout**: a JavaScript `keydown` listener captures layout-aware characters (e.g. AZERTY) because Allegro's Emscripten backend always reports QWERTY key codes.
- **Fullscreen**: uses the browser Fullscreen API; the `on_fullscreen_change` callback handles display resize.

## Android Build

### APK vs AAB -- which one do you need?

| Format | Target | Output file | Use for |
|---|---|---|---|
| Debug APK | `make android` | `BlockBlaster-debug.apk` | Development, sideloading, local testing via `adb install` |
| Release APK | `make android-release` | `BlockBlaster-release.apk` | Internal test tracks, direct APK distribution |
| Release AAB | `make android-aab` | `BlockBlaster-release.aab` | **Google Play Store submission** |

> **Why AAB for the Play Store?**
> Since August 2021, Google Play requires the **Android App Bundle (AAB)** format for new app submissions.
> An AAB is not directly installable; Google Play processes it server-side and generates
> device-optimised APKs (matching ABI, screen density, and language) for each user.
> The result is a smaller download for end-users without any work on your side.
> APKs remain useful for development and sideloading.

### Requirements

| Tool | Required for | Notes |
|---|---|---|
| Android SDK | all targets | Default path `~/Android/Sdk`, set `ANDROID_SDK_ROOT` to override |
| Android NDK | all targets | Auto-detected inside `$ANDROID_SDK_ROOT/ndk/` |
| `cmake` | native lib build | Part of Android SDK or system package |
| `javac`, `d8` | DEX compilation | Part of Android SDK build-tools |
| `keytool` | debug keystore | Bundled with any JDK |
| `jarsigner` | AAB signing | Bundled with any JDK |
| `aapt2` | AAB only | Part of Android SDK build-tools (same folder as `aapt`) |
| `bundletool.jar` | AAB only | Download from [github.com/google/bundletool/releases](https://github.com/google/bundletool/releases), place as `bundletool.jar` in project root (or set `BUNDLETOOL_JAR=<path>`) |

### Configurable variables

| Variable | Default | Description |
|---|---|---|
| `ANDROID_SDK_ROOT` | `~/Android/Sdk` | Path to Android SDK |
| `ANDROID_NDK` | auto-detected | Path to NDK (inside SDK) |
| `ANDROID_API` | `24` | Minimum API level (Android 7.0) |
| `ANDROID_ABI` | `arm64-v8a` | Single ABI for per-ABI sub-targets |
| `ANDROID_ABIS` | `armeabi-v7a arm64-v8a` | All ABIs compiled by `android-native-all` |
| `ANDROID_PACKAGE` | `org.gullradriel.blockblaster` | App package name |
| `ANDROID_VERSION_CODE` | `1` | Integer version, must increment with each Play Store upload |
| `ANDROID_VERSION_NAME` | `1.0.0` | Human-readable version string |
| `ANDROID_FINAL_APK` | `BlockBlaster-debug.apk` | Debug APK output path |
| `ANDROID_FINAL_RELEASE_APK` | `BlockBlaster-release.apk` | Release APK output path |
| `ANDROID_FINAL_RELEASE_AAB` | `BlockBlaster-release.aab` | Release AAB output path |
| `BUNDLETOOL_JAR` | `bundletool.jar` | Path to the bundletool JAR |
| `ANDROID_JAVA_RELEASE` | `11` | Java source/target release level |

Supported ABI values: `armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`.

### First-time setup

#### 1. Launcher icons

The manifest references `@mipmap/ic_launcher`. PNG files must exist in
`android/res/mipmap-{mdpi,hdpi,xhdpi,xxhdpi,xxxhdpi}/` before packaging.

Generate placeholder icons (requires ImageMagick):

```sh
make android-gen-icons
```

Replace the generated PNGs with real artwork before submitting to the Play Store.

Alternatively, use the included Python script to generate icons from a source image:

```sh
python3 gen_icon.py source_icon.png
```

#### 2. Release keystore (release APK and AAB only)

Generate a release keystore **once** and keep it safe -- you cannot re-sign an existing
Play Store app with a different key:

```sh
keytool -genkey -v \
  -keystore android/release.keystore \
  -alias blockblaster-release \
  -keyalg RSA -keysize 4096 -validity 10000
```

Then create `keystore.properties` from the template and fill in your values:

```sh
cp keystore.properties.template keystore.properties
# edit keystore.properties -- set ANDROID_RELEASE_ALIAS, _STOREPASS, _KEYPASS
```

`keystore.properties` and `android/release.keystore` are gitignored and must never be
committed to version control.

#### 3. bundletool (AAB only)

Download `bundletool-all-<version>.jar` from
[github.com/google/bundletool/releases](https://github.com/google/bundletool/releases)
and rename it to `bundletool.jar` in the project root (also gitignored).

### Targets

| Target | Description |
|---|---|
| `make android-setup` | Validates Android SDK/NDK/build-tools environment |
| `make android-gen-icons` | Generates placeholder launcher icons (requires ImageMagick) |
| `make android-libogg` | Builds `libogg` for one ABI |
| `make android-libvorbis` | Builds `libvorbis` for one ABI |
| `make android-libfreetype` | Builds `freetype` for one ABI |
| `make android-allegro` | Builds Allegro 5 monolith for one ABI |
| `make android-native` | Compiles `libblockblaster.so` for one ABI |
| `make android-native-all` | Builds native lib for all ABIs in `ANDROID_ABIS` |
| `make android-dex` | Compiles Java activity + Allegro Java sources to `classes.dex` |
| `make android-keystore` | Generates the debug keystore if not present |
| `make android` | Full **debug APK** build |
| `make android-release` | Full **release APK** build |
| `make android-aab` | Full **release AAB** build (Play Store) |
| `make android-apk-path` | Prints the debug APK path |
| `make android-clean` | Removes all Android build artifacts and output files |

### Building

**Debug APK** (development / sideloading):

```sh
make android
adb install BlockBlaster-debug.apk
```

**Release AAB** (Play Store submission):

```sh
# First time only:
make android-gen-icons          # generate placeholder icons
# ... then replace android/res/mipmap-*/ic_launcher*.png with real art ...
# generate release.keystore + fill keystore.properties
# download bundletool.jar

make android-aab \
  ANDROID_VERSION_CODE=1 \
  ANDROID_VERSION_NAME=1.0.0
# Upload BlockBlaster-release.aab to Google Play Console.
```

**Release APK** (internal testing / sideload distribution):

```sh
make android-release \
  ANDROID_VERSION_CODE=1 \
  ANDROID_VERSION_NAME=1.0.0
```

### Android-specific behaviour

- **Touch input**: uses Allegro's mouse emulation (`ALLEGRO_MOUSE_EMULATION_TRANSPARENT`) so touch events are processed as mouse events.
- **Finger offset**: the dragged piece is shifted 70 virtual pixels upward so it is not hidden under the player's finger.
- **Display density**: the font size is scaled by the device's display density (queried via JNI) to remain readable on high-DPI screens.
- **Soft keyboard**: shown automatically on the game-over name entry screen; tapping the name field re-opens it.
- **Halt/resume**: when the app is backgrounded, audio is paused and the timer is stopped; on resume everything is restored.
- **APK file interface**: `al_android_set_apk_file_interface()` is called so that Allegro can read assets directly from the APK.

## Clean Targets

| Target | Description |
|---|---|
| `make clean` | Remove native build artifacts (objects + binary) |
| `make wasm-clean` | Remove WASM build directory and dependency builds |
| `make android-clean` | Remove Android build directory, APK, and AAB |
| `make clean-all` | Run all three clean targets |

## Code Formatting

The project uses `clang-format` for consistent code style. Run:

```sh
./format-code.sh
```

This formats all `.c` and `.h` files in `src/` according to the `.clang-format` configuration.
