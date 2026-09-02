# iptvPlus
Video Player for ```.m3u``` Files and Movies, Series is Coming Soon

### build Visual Studio 2026

### Config

Before run compilation command, you need ``libmpv-2.dll``, that is include in ``3rdparty/mpv`` in 4 compressed files.

```sh
cmake -S . -B build `
  -G "Visual Studio 18 2026" `
  "-DQt6_DIR=C:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6" `
  "-DCMAKE_INSTALL_PREFIX=build/install"

# you can use Debug mode too
cmake --build build --config Release
cmake --install build --config Release

# Only if you need the installer
cpack --config build/CPackConfig.cmake -C Release
```

### 3dpartys
- mpv (videoplayer)

![screenshoot](/screenshoot.png "IPTV ++ Screenshoot")
