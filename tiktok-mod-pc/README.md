# TikTok Mod PC

A Windows x64 launcher for the Android TikTok Mod release 46.4.3. It downloads the APK on demand, verifies its published SHA-256 and size, installs it through ADB, and launches it inside an already-installed Android emulator.

## Important: this is not a native Windows port

The supplied file is an Android APK. It contains no Windows source project, and the upstream repository is a TypeScript/Bun CLI for retrieving Android releases. Converting that APK into a native Win32 TikTok client is not possible from the APK alone. This EXE is a Windows control panel for the real Android build running in an emulator.

## Requirements

- Windows 10/11 x64.
- An Android emulator installed separately (for example, an emulator that exposes ADB).
- ADB debugging enabled in the emulator.
- Internet access for the first APK download.
- Around 500 MB free for the APK, plus emulator storage.

## Use

1. Install and start your Android emulator.
2. Enable its ADB/debugging option.
3. Run TikTokModPC.exe.
4. Click **Скачать APK**. The app checks the exact file size and SHA-256 before keeping the download.
5. Click **Проверить ADB**. If ADB is not found automatically, choose adb.exe or HD-Adb.exe.
6. Select the connected emulator, click **Установить / обновить**, then **Запустить TikTok Mod**.

If more than one Android device is connected, select the intended emulator in the device list. The package name can be edited if the APK uses a different Android package ID.

## Pinned release file

- Release: https://github.com/namecallfilter/tiktokmodcloud/releases/tag/v46.4.3-mod-fix
- APK: 46.4.3_universal_fix.apk
- Size: 501809029 bytes
- SHA-256: 88845094adb702e2b92ada1fec56920083208ff48db03a879621f3de8a14190c

The APK is downloaded at runtime and is not redistributed inside this repository or EXE. A matching checksum proves that the downloaded bytes match the release metadata; it does **not** prove that the third-party mod is safe or trustworthy. The app has not been independently security-audited. Avoid using your primary account or entering sensitive credentials in third-party modified apps.

## Build

Open this project on Windows with .NET 8 SDK. Run:

    dotnet publish .\TikTokModPC.csproj -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o .\publish

The output is publish\TikTokModPC.exe.
