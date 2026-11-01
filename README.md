# Toker — Local Luau Workspace

A native Windows desktop editor and runner for **local Luau scripts**. It is a standalone scripting workspace, not a Roblox process executor: it does not attach to Roblox, inject DLLs, modify game memory, or bypass anti-cheat.

## Features

- Native C++20 Win32 desktop application.
- Embedded Luau compiler and VM (fetched from the official Luau repository at configure time).
- Script editor, open/save dialogs, run button, and output console.
- UTF-8 source handling and basic file-size limits.
- GitHub Actions build for Windows x64.

## Build on Windows

Requirements:
- Visual Studio 2022 with **Desktop development with C++**.
- CMake 3.24 or newer.
- Git installed and available on PATH.
- Internet access during CMake configure so FetchContent can download Luau.

Run from a Developer PowerShell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
.\build\Release\Toker.exe
```

## GitHub Actions

Every push to `main` runs the Windows build. Open **Actions → Build Toker Windows** and download the `Toker-Windows-x64` artifact after a successful run.

## Notes

- Scripts run in a local Luau VM, not inside Roblox.
- Execution currently runs on the UI thread. A script with a long-running or infinite loop can make the window unresponsive; close and restart the app if that happens.
- The Luau dependency is fetched from the upstream `master` branch during configuration, so builds use the upstream revision available at build time.
