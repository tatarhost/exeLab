# WorkInk Browser

A small Windows browser window that opens https://work.ink/ by default.

## Included
- Chromium engine supplied by Electron.
- Back, forward, reload, home, and address bar.
- Separate windows for ordinary HTTP/HTTPS pop-ups.
- Standard downloads with a Save As dialog.
- No built-in ad blocker, tracker filter, proxy, or bundled extensions.

The app does not alter the site's scripts, forge browser-check results, or disable Chromium's security protections. It uses Electron's normal user agent and normal web security settings. A website may still apply its own compatibility or access checks.

## Build
The GitHub Actions workflow builds a portable Windows x64 executable named WorkInk-Browser-1.0.0-portable.exe.
