reaper_libsndfilewrapper
========================

REAPER extension that adds import (reading) and export (writing) for the audio
formats handled by [libsndfile](http://libsndfile.github.io/libsndfile/) —
RF64, CAF, W64, AIFF, AU, PAF, SD2, VOC, RAW and more — to the
[REAPER](https://www.reaper.fm) digital audio workstation.

Adds read/write support for soundfiles such as **RF64** (e.g. RME DIGICheck,
mh acoustics Eigenstudio®), `.caf`, `.w64`, `.paf`, `.sf`, `.raw`, `.au`, ...

Supported file extensions include: `.au`, `.avr`, `.caf`, `.htk`, `.iff`,
`.mat`, `.mpc`, `.paf`, `.pvf`, `.raw`, `.rf64`, `.sd2`, `.sds`, `.sf`, `.voc`,
`.w64`, `.wve`, `.xi`

> libsndfile is built **core-formats-only** (no external codec libraries), so
> compressed formats that REAPER already handles natively (FLAC, Ogg/Vorbis,
> Opus, MP3) are intentionally not included here.


Releases
--------

Installers for Windows (`.exe`), macOS (`.pkg`) and Linux (`.tar.gz`) are built
automatically and published in
[GitHub Releases](https://github.com/kronihias/reaper_libsndfilewrapper/releases).

- **Windows**: run the `_win64_setup.exe` — installs `reaper_libsndfilewrapper.dll`
  into `%APPDATA%\REAPER\UserPlugins`.
- **macOS**: open the `_macos.pkg` — installs the universal (Apple Silicon +
  Intel) `reaper_libsndfilewrapper.dylib` into
  `/Library/Application Support/REAPER/UserPlugins`.
- **Linux**: extract the `.tar.gz` and run `./install.sh` (copies
  `reaper_libsndfilewrapper.so` into `~/.config/REAPER/UserPlugins`).

Restart REAPER after installing.

To cut a new release: bump the `VERSION` file, commit, and publish a GitHub
release tagged `vX.Y.Z` (matching `VERSION`). Publishing triggers the build +
signing workflow. Code-signing setup is documented in
[.github/CODE_SIGNING.md](.github/CODE_SIGNING.md).


Building from source
--------------------

Requirements:

- CMake 3.10+ and a C++ toolchain
- macOS: Xcode command-line tools
- Windows: Visual Studio 2022 (Desktop development with C++)
- Linux: `build-essential`

`WDL` and `libsndfile` are vendored as git submodules and **statically linked**
— no external audio libraries are required at build or run time.

```sh
git clone --recursive https://github.com/kronihias/reaper_libsndfilewrapper
cd reaper_libsndfilewrapper
./scripts/setup.sh        # init submodules + cmake configure
cmake --build build-dev   # on macOS, writes the plugin straight into REAPER's UserPlugins
```

For installer builds:

```sh
./scripts/build_osx.sh      # macOS  → _OSX_RELEASE/...macos.pkg     (codesign/notarize; add --no-sign to skip)
scripts\build_win.bat       # Windows → _WIN_RELEASE/..._win64_setup.exe
./scripts/build_linux.sh    # Linux  → _LINUX_RELEASE/..._linux_x86_64.tar.gz
```


License
-------

This plug-in is licensed under the **GNU Lesser General Public License (LGPL)**;
see [COPYING](COPYING).

It statically links **libsndfile** by Erik de Castro Lopo, which is also LGPL
(see `libsndfile/COPYING`, shipped in the installers). Because the full source
of this project — including the pinned libsndfile submodule — is publicly
available, a recipient can rebuild against a modified libsndfile, satisfying the
LGPL's relinking requirement.


Credits
-------

- Originally written by **Xenakios** (https://xenakios.wordpress.com)
- Bugfixes, export functionality, and macOS/Windows/Linux builds by
  **Matthias Kronlachner** — m.kronlachner (ät) gmail.com — www.matthiaskronlachner.com
- *libsndfile* by Erik de Castro Lopo (LGPL) — http://libsndfile.github.io/libsndfile/
- *WDL/swell* by Cockos Inc. — https://www.cockos.com/wdl/
