#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <algorithm>

#include "lua.h"
#include "lualib.h"
#include "Luau/Compiler.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")

namespace {
constexpr int ID_EDITOR = 1001;
constexpr int ID_OUTPUT = 1002;
constexpr int ID_RUN = 1003;
constexpr int ID_CLEAR = 1004;
constexpr int ID_OPEN = 1005;
constexpr int ID_SAVE = 1006;
constexpr int ID_STATUS = 1007;
constexpr UINT WM_RUN_FINISHED = WM_APP + 10;

HWND g_window{}, g_editor{}, g_output{}, g_status{}, g_run{};
HFONT g_uiFont{}, g_codeFont{};
HBRUSH g_background{}, g_panel{}, g_editorBrush{};
bool g_running = false;

const COLORREF BG = RGB(13, 15, 19);
const COLORREF PANEL = RGB(21, 24, 30);
const COLORREF TEXT = RGB(232, 235, 241);
const COLORREF MUTED = RGB(137, 146, 160);
const COLORREF ACCENT = RGB(117, 155, 255);

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                static_cast<int>(value.size()), nullptr, 0);
    if (!n) return L"[output encoding error]";
    std::wstring result(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), result.data(), n);
    return result;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (!n) return {};
    std::string result(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), n, nullptr, nullptr);
    return result;
}

void AppendOutput(const std::wstring& text) {
    if (!g_output) return;
    const int length = GetWindowTextLengthW(g_output);
    SendMessageW(g_output, EM_SETSEL, length, length);
    SendMessageW(g_output, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    SendMessageW(g_output, EM_SCROLLCARET, 0, 0);
}

void SetStatus(const std::wstring& value) {
    if (g_status) SetWindowTextW(g_status, value.c_str());
}

void AddPrint(lua_State* state) {
    auto* output = static_cast<std::wstring*>(lua_touserdata(state, lua_upvalueindex(1)));
    const int count = lua_gettop(state);
    std::wstring line;
    for (int i = 1; i <= count; ++i) {
        size_t length = 0;
        const char* value = luaL_tolstring(state, i, &length);
        if (i > 1) line += L"\t";
        if (value) line += Utf8ToWide(std::string(value, length));
        lua_pop(state, 1);
    }
    line += L"\r\n";
    if (output) *output += line;
}

struct RunResult {
    std::wstring output;
    bool success = false;
};

RunResult ExecuteLuau(const std::wstring& source) {
    RunResult result;
    const std::string utf8 = WideToUtf8(source);
    if (utf8.empty() && !source.empty()) {
        result.output = L"Could not convert source text to UTF-8.\r\n";
        return result;
    }

    try {
        const std::string bytecode = Luau::compile(utf8);
        if (bytecode.empty()) {
            result.output = L"Compiler returned empty bytecode.\r\n";
            return result;
        }

        lua_State* state = luaL_newstate();
        if (!state) {
            result.output = L"Could not create Luau VM state.\r\n";
            return result;
        }

        luaL_openlibs(state);
        lua_pushlightuserdata(state, &result.output);
        lua_pushcclosure(state, AddPrint, "print", 1);
        lua_setglobal(state, "print");

        const int loadStatus = luau_load(state, "toker-script", bytecode.data(),
                                         bytecode.size(), 0);
        if (loadStatus != 0) {
            const char* message = lua_tostring(state, -1);
            result.output += L"Load error: ";
            result.output += message ? Utf8ToWide(message) : L"unknown error";
            result.output += L"\r\n";
            lua_close(state);
            return result;
        }

        const int callStatus = lua_pcall(state, 0, LUA_MULTRET, 0);
        if (callStatus != 0) {
            const char* message = lua_tostring(state, -1);
            result.output += L"Runtime error: ";
            result.output += message ? Utf8ToWide(message) : L"unknown error";
            result.output += L"\r\n";
            lua_close(state);
            return result;
        }

        lua_close(state);
        result.success = true;
        if (result.output.empty()) result.output = L"Script finished without console output.\r\n";
    } catch (const std::exception& error) {
        result.output += L"Compiler exception: ";
        result.output += Utf8ToWide(error.what());
        result.output += L"\r\n";
    } catch (...) {
        result.output += L"Unknown execution error.\r\n";
    }
    return result;
}

void ApplyFont(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

HWND MakeButton(HWND parent, int id, const wchar_t* label, int x, int width) {
    HWND button = CreateWindowExW(0, L"BUTTON", label,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, 16, width, 34, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    ApplyFont(button, g_uiFont);
    return button;
}

void OpenFile() {
    wchar_t path[MAX_PATH]{};
    OPENFILENAMEW dialog{ sizeof(dialog) };
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = L"Luau scripts (*.luau;*.lua)\0*.luau;*.lua\0Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&dialog)) {
        HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            MessageBoxW(g_window, L"Could not open the selected file.", L"Toker", MB_ICONERROR);
            return;
        }
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size) || size.QuadPart > 4 * 1024 * 1024) {
            CloseHandle(file);
            MessageBoxW(g_window, L"File is too large (limit: 4 MB).", L"Toker", MB_ICONWARNING);
            return;
        }
        std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
        DWORD read = 0;
        const BOOL ok = bytes.empty() || ReadFile(file, bytes.data(),
            static_cast<DWORD>(bytes.size()), &read, nullptr);
        CloseHandle(file);
        if (!ok) {
            MessageBoxW(g_window, L"Could not read the selected file.", L"Toker", MB_ICONERROR);
            return;
        }
        bytes.resize(read);
        SetWindowTextW(g_editor, Utf8ToWide(bytes).c_str());
        SetStatus(L"Loaded script");
    }
}

void SaveFile() {
    wchar_t path[MAX_PATH] = L"script.luau";
    OPENFILENAMEW dialog{ sizeof(dialog) };
    dialog.hwndOwner = g_window;
    dialog.lpstrFilter = L"Luau scripts (*.luau)\0*.luau\0Lua scripts (*.lua)\0*.lua\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = L"luau";
    if (!GetSaveFileNameW(&dialog)) return;

    const int chars = GetWindowTextLengthW(g_editor);
    std::wstring source(static_cast<size_t>(chars) + 1, L'\0');
    GetWindowTextW(g_editor, source.data(), chars + 1);
    source.resize(chars);
    const std::string bytes = WideToUtf8(source);

    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        MessageBoxW(g_window, L"Could not save the file.", L"Toker", MB_ICONERROR);
        return;
    }
    DWORD written = 0;
    const BOOL ok = bytes.empty() || WriteFile(file, bytes.data(),
        static_cast<DWORD>(bytes.size()), &written, nullptr);
    CloseHandle(file);
    if (!ok || written != bytes.size()) {
        MessageBoxW(g_window, L"File write was incomplete.", L"Toker", MB_ICONERROR);
        return;
    }
    SetStatus(L"Saved script");
}

void RunScript() {
    if (g_running) return;
    const int chars = GetWindowTextLengthW(g_editor);
    if (chars > 1024 * 1024) {
        MessageBoxW(g_window, L"Script exceeds the 1 MB execution limit.", L"Toker", MB_ICONWARNING);
        return;
    }
    std::wstring source(static_cast<size_t>(chars) + 1, L'\0');
    GetWindowTextW(g_editor, source.data(), chars + 1);
    source.resize(chars);

    SetWindowTextW(g_output, L"");
    AppendOutput(L"> Compiling and running local Luau script...\r\n");
    g_running = true;
    EnableWindow(g_run, FALSE);
    SetStatus(L"Running");
    RunResult result = ExecuteLuau(source);
    AppendOutput(result.output);
    AppendOutput(result.success ? L"\r\n[OK] Execution completed.\r\n" : L"\r\n[ERROR] Execution failed.\r\n");
    g_running = false;
    EnableWindow(g_run, TRUE);
    SetStatus(result.success ? L"Ready" : L"Execution error");
}

void Layout(int width, int height) {
    const int margin = 22;
    const int top = 66;
    const int outputHeight = std::max(125, height / 4);
    if (g_editor) MoveWindow(g_editor, margin, top + 24, width - margin * 2,
                              height - outputHeight - top - 74, TRUE);
    if (g_output) MoveWindow(g_output, margin, height - outputHeight - 34,
                              width - margin * 2, outputHeight, TRUE);
    HWND editorLabel = GetDlgItem(g_window, 1010);
    HWND outputLabel = GetDlgItem(g_window, 1011);
    if (editorLabel) MoveWindow(editorLabel, margin, top, 250, 20, TRUE);
    if (outputLabel) MoveWindow(outputLabel, margin, height - outputHeight - 58, 250, 20, TRUE);
    if (g_status) MoveWindow(g_status, margin, height - 25, width - margin * 2, 18, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        g_uiFont = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_codeFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, L"Cascadia Code");
        g_background = CreateSolidBrush(BG);
        g_panel = CreateSolidBrush(PANEL);
        g_editorBrush = CreateSolidBrush(RGB(17, 20, 26));

        g_run = MakeButton(hwnd, ID_RUN, L"▶  RUN", 22, 100);
        MakeButton(hwnd, ID_OPEN, L"Open", 132, 76);
        MakeButton(hwnd, ID_SAVE, L"Save", 218, 76);
        MakeButton(hwnd, ID_CLEAR, L"Clear output", 304, 112);

        HWND editorLabel = CreateWindowExW(0, L"STATIC", L"SCRIPT EDITOR  /  LOCAL LUAU",
            WS_CHILD | WS_VISIBLE, 22, 66, 300, 20, hwnd,
            reinterpret_cast<HMENU>(1010), GetModuleHandleW(nullptr), nullptr);
        HWND outputLabel = CreateWindowExW(0, L"STATIC", L"OUTPUT CONSOLE",
            WS_CHILD | WS_VISIBLE, 22, 500, 250, 20, hwnd,
            reinterpret_cast<HMENU>(1011), GetModuleHandleW(nullptr), nullptr);
        ApplyFont(editorLabel, g_uiFont);
        ApplyFont(outputLabel, g_uiFont);

        g_editor = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT",
            L"-- Toker local Luau workspace\r\nprint(\"Hello from Toker!\")\r\n\r\nlocal total = 20 + 22\r\nprint(\"Answer:\", total)",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL,
            22, 90, 500, 300, hwnd, reinterpret_cast<HMENU>(ID_EDITOR),
            GetModuleHandleW(nullptr), nullptr);
        ApplyFont(g_editor, g_codeFont);
        SendMessageW(g_editor, EM_SETLIMITTEXT, 1024 * 1024, 0);

        g_output = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Ready.\r\n",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_READONLY,
            22, 500, 500, 130, hwnd, reinterpret_cast<HMENU>(ID_OUTPUT),
            GetModuleHandleW(nullptr), nullptr);
        ApplyFont(g_output, g_codeFont);
        SendMessageW(g_output, EM_SETLIMITTEXT, 256 * 1024, 0);

        g_status = CreateWindowExW(0, L"STATIC", L"Ready  •  Local runtime",
            WS_CHILD | WS_VISIBLE, 22, 700, 500, 18, hwnd,
            reinterpret_cast<HMENU>(ID_STATUS), GetModuleHandleW(nullptr), nullptr);
        ApplyFont(g_status, g_uiFont);
        return 0;
    }
    case WM_SIZE:
        Layout(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_RUN: RunScript(); return 0;
        case ID_OPEN: OpenFile(); return 0;
        case ID_SAVE: SaveFile(); return 0;
        case ID_CLEAR: SetWindowTextW(g_output, L""); SetStatus(L"Console cleared"); return 0;
        }
        break;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetTextColor(dc, (control == g_status) ? MUTED : TEXT);
        SetBkColor(dc, (control == g_editor || control == g_output) ? RGB(17, 20, 26) : BG);
        return reinterpret_cast<LRESULT>((control == g_editor || control == g_output) ? g_editorBrush : g_background);
    }
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, TEXT);
        SetBkColor(dc, PANEL);
        return reinterpret_cast<LRESULT>(g_panel);
    }
    case WM_DESTROY:
        if (g_uiFont) DeleteObject(g_uiFont);
        if (g_codeFont) DeleteObject(g_codeFont);
        if (g_background) DeleteObject(g_background);
        if (g_panel) DeleteObject(g_panel);
        if (g_editorBrush) DeleteObject(g_editorBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    INITCOMMONCONTROLSEX controls{ sizeof(controls), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&controls);

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG);
    wc.lpszClassName = L"TokerExecutorWindow";
    if (!RegisterClassExW(&wc)) return 1;

    g_window = CreateWindowExW(0, wc.lpszClassName, L"TOKER  /  Luau Workspace",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1050, 760,
        nullptr, nullptr, instance, nullptr);
    if (!g_window) return 2;

    ShowWindow(g_window, showCommand);
    UpdateWindow(g_window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
