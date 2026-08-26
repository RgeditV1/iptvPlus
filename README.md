# iptvPlus
Video Player for ```.m3u``` Files and Movies, Series is Coming Soon

### build Visual Studio 2026

### Config
```sh
cmake -S . -B build `
  -G "Visual Studio 18 2026" `
  "-DQt6_DIR=C:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6" `
  "-DCMAKE_INSTALL_PREFIX=build/install"

cmake --build build --config Release
cmake --install build --config Release
cpack --config build/CPackConfig.cmake -C Release
```

### 3dpartys
- mpv (for the reproducer)

![screenshoot](/screenshoot.png "IPTV ++ Screenshoot")
