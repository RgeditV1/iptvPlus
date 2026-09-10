# iptvPlus
Video Player for ```.m3u``` Files and Movies, Series is Coming Soon

![screenshoot](/screenshoot.png "IPTV ++ Screenshoot")

### Build

Before run compilation command, you need ``libmpv-2.dll``, that is include in ``3rdparty/mpv`` in 4 compressed files.

```sh
cmake -S . -B build `
  -G "Visual Studio 18 2026" -A x64 `
  "-DQt6_DIR=C:/Qt/6.11.2/msvc2022_64/lib/cmake/Qt6" `
  "-DCMAKE_INSTALL_PREFIX=build/install" `
  "-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake" # optional, need vcpkg installed

# you can use Debug mode too
cmake --build build --config Release
cmake --install build --config Release

# Only if you need the installer
cpack --config build/CPackConfig.cmake -C Release
```

### 3rdpartys

- ``mpv`` (videoplayer)
- ``libtorrent`` (and submodules)
- ``vcpkg`` for lib magnaments (optional but recomended)
- ``OpenSSL`` (should use vcpkg)
- ``Boost`` (should use vcpkg)


