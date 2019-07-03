# Animatch

Animatch is a match-three game with cute animals.

The game was made for [Purism, SPC](https://puri.sm/) by [Holy Pangolin](https://holypangolin.com/) studio, consisting of [Agata Nawrot](https://agatanawrot.com/) and [Sebastian Krzyszkowiak](https://dosowisko.net/).

## Cloning the repository

As Animatch uses git submodules, make sure to initialize them before trying to compile:

```
git clone https://gitlab.com/HolyPangolin/animatch
cd animatch
git submodule update --init --recursive
```

## Compiling

### Natively on GNU/Linux

You need a complete set of *Allegro 5* libraries, *CMake* and, of course, a C/C++ compiler installed. *Ninja* build system is optional, but recommended. On Debian-based distributions (Ubuntu, PureOS, ...), you can install all dependencies by issuing:

```
sudo apt install build-essential cmake liballegro5-dev ninja-build
```

On Arch:

```
pacman -S --needed base-devel cmake allegro ninja
```

To build Animatch:

```
mkdir build
cd build
cmake .. -GNinja
ninja
```

Now you can run the game by calling:

```
src/animatch
```

### Using Flatpak

You need to have *flatpak-builder* installed.

To build and install Animatch:

```
flatpak-builder build-flatpak flatpak/com.holypangolin.Animatch.json --force-clean --install --user
```

To run Animatch after building the flatpak, issue this command:

```
flatpak run com.holypangolin.Animatch
```

The game depends on the Freedesktop SDK 18.08. If Flatpak doesn't offer to install it automatically when building, make sure you have some repository that contains it enabled, for instance Flathub:

```
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

**Note**: Flatpak builds configure Allegro to use SDL2 backend, which gives them native Wayland support.

### Using Docker SDK

You can cross-compile Animatch to a bunch of various platforms using libsuperderpy's Docker SDK. To do so, you can run various `package_*.sh` scripts from the `utils/` directory. The resulting packages will be placed into `utils/output/`. For more information, consult the [utils/README](https://gitlab.com/dosowisko.net/libsuperderpy-utils/blob/master/README) file.

## Compilation options

You can pass some variables to CMake in order to configure the way in which Animatch will be built:

|Variable|Meaning|
|--------|-------|
|`BUILD_SHARED_LIBS` | enabled by default; implies `LIBSUPERDERPY_STATIC`, `LIBSUPERDERPY_STATIC_COMMON` and `LIBSUPERDERPY_STATIC_GAMESTATES` when disabled |
|`LIBSUPERDERPY_STATIC` | links the libsuperderpy engine as a static library |
|`LIBSUPERDERPY_STATIC_COMMON` | links the common Animatch routines as a static library |
|`LIBSUPERDERPY_STATIC_GAMESTATES` | links the gamestates as static libraries |
|`LIBSUPERDERPY_STATIC_DEPS` | tries to link Allegro in statically |
|`LIBSUPERDERPY_LTO` | enables link-time optimizations |
|`USE_CLANG_TIDY` | when enabled, uses clang-tidy for static analyzer warnings when compiling. |
|`SANITIZERS` | enables one or more kinds of compiler instrumentation: address, undefined, leak, thread |

Example: `cmake .. -GNinja -DLIBSUPERDERPY_LTO=ON`

### License

The game is available under the terms of [GNU General Public License 3.0](COPYING) or later.

### Contributing

Animatch uses the libsuperderpy's code style guide: [libsuperderpy/README_codestyle.md](https://gitlab.com/dosowisko.net/libsuperderpy/blob/master/README_codestyle.md).
