#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <string>

namespace {
constexpr wchar_t kClassName[] = L"TapTapDashPcLauncherWindow";
constexpr wchar_t kGameUrl[] = L"https://play.google.com/pc-store/games/details?id=com.secondarm.taptapdash";
constexpr wchar_t kGooglePlayGamesUrl[] = L"https://play.google.com/googleplaygames";

constexpr int ID_OPEN_GAME = 1001;
constexpr int ID_INSTALL_GPG = 1002;
constexpr int ID_REQUIREMENTS = 1003;

HINSTANCE g_instance = nullptr;
HFONT g_title_font = nullptr;
HFONT g_subtitle_font = nullptr;
HFONT g_body_font = nullptr;
HFONT g_small_font = nullptr;
HBRUSH g_background_brush = nullptr;

void OpenUrl(HWND owner, const wchar_t* url) {
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteW(owner, L"open", url, nullptr, nullptr, SW_SHOWNORMAL));
    if (result <= 32) {
        MessageBoxW(owner,
            L"Не удалось открыть ссылку. Проверь подключение к интернету и браузер.",
            L"Tap Tap Dash PC", MB_OK | MB_ICONWARNING);
    }
}

HWND AddLabel(HWND parent, const wchar_t* text, int x, int y, int width,
              int height, HFONT font) {
    HWND control = CreateWindowExW(
        0, L"STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, width, height, parent, nullptr, g_instance, nullptr);
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

HWND AddButton(HWND parent, const wchar_t* text, int id, int x, int y,
               int width, int height) {
    HWND control = CreateWindowExW(
        0, L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
        x, y, width, height, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        g_instance, nullptr);
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_body_font), TRUE);
    return control;
}

void ShowRequirements(HWND owner) {
    MessageBoxW(owner,
        L"Для официальной версии Tap Tap Dash на ПК через Google Play Games "
        L"указаны следующие минимальные требования:\n\n"
        L"• Windows 10 (версия 2004) или новее\n"
        L"• 8 ГБ оперативной памяти\n"
        L"• SSD с 10 ГБ свободного места\n"
        L"• Процессор минимум с 4 физическими ядрами\n"
        L"• Включённая аппаратная виртуализация\n\n"
        L"Точные требования и доступность могут меняться. Проверь их "
        L"на официальной странице Google Play Games.",
        L"Системные требования", MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        AddLabel(window, L"TAP TAP DASH", 36, 30, 660, 48, g_title_font);
        AddLabel(window, L"PC LAUNCHER", 38, 78, 660, 25, g_subtitle_font);
        AddLabel(window, L"Официальный запуск на Windows через Google Play Games",
                 38, 132, 680, 30, g_body_font);

        AddLabel(window, L"КАК ЭТО РАБОТАЕТ", 38, 206, 680, 24, g_subtitle_font);
        AddLabel(window,
            L"1. Установи Google Play Games для Windows.\n"
            L"2. Открой страницу Tap Tap Dash и установи игру.\n"
            L"3. Запускай её из библиотеки Google Play Games.",
            38, 242, 680, 88, g_body_font);

        AddLabel(window, L"ВАЖНО", 38, 345, 680, 22, g_subtitle_font);
        AddLabel(window,
            L"Этот EXE — лаунчер, а не самостоятельная копия игры. "
            L"Игру запускает официальный клиент Google Play Games.",
            38, 374, 680, 46, g_small_font);

        AddButton(window, L"ОТКРЫТЬ TAP TAP DASH", ID_OPEN_GAME, 38, 444, 310, 48);
        AddButton(window, L"УСТАНОВИТЬ GOOGLE PLAY GAMES", ID_INSTALL_GPG, 366, 444, 350, 48);
        AddButton(window, L"Системные требования", ID_REQUIREMENTS, 38, 506, 210, 34);
        AddLabel(window, L"Официальная страница игры · JOX Development LLC",
                 270, 512, 450, 24, g_small_font);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case ID_OPEN_GAME:
            OpenUrl(window, kGameUrl);
            return 0;
        case ID_INSTALL_GPG:
            OpenUrl(window, kGooglePlayGamesUrl);
            return 0;
        case ID_REQUIREMENTS:
            ShowRequirements(window);
            return 0;
        }
        break;

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wparam);
        SetTextColor(dc, RGB(226, 234, 242));
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(g_background_brush);
    }

    case WM_DRAWITEM: {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);
        if (!item) break;

        const bool primary = item->CtlID == ID_OPEN_GAME;
        const bool pressed = (item->itemState & ODS_SELECTED) != 0;
        COLORREF fill = primary ? RGB(29, 172, 139) : RGB(48, 79, 112);
        if (pressed) fill = primary ? RGB(22, 137, 111) : RGB(38, 63, 91);

        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, fill);
        HGDIOBJ old_brush = SelectObject(item->hDC, brush);
        HGDIOBJ old_pen = SelectObject(item->hDC, pen);
        RoundRect(item->hDC, item->rcItem.left + 1, item->rcItem.top + 1,
                  item->rcItem.right - 1, item->rcItem.bottom - 1, 12, 12);

        wchar_t text[128]{};
        GetWindowTextW(item->hwndItem, text, 128);
        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, RGB(255, 255, 255));
        SelectObject(item->hDC, g_body_font);
        RECT text_rect = item->rcItem;
        DrawTextW(item->hDC, text, -1, &text_rect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        SelectObject(item->hDC, old_pen);
        SelectObject(item->hDC, old_brush);
        DeleteObject(pen);
        DeleteObject(brush);
        return TRUE;
    }

    case WM_ERASEBKGND: {
        RECT rect{};
        GetClientRect(window, &rect);
        FillRect(reinterpret_cast<HDC>(wparam), &rect, g_background_brush);
        return 1;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    g_instance = instance;
    g_background_brush = CreateSolidBrush(RGB(13, 20, 31));
    g_title_font = CreateFontW(34, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_subtitle_font = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_body_font = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_small_font = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = g_background_brush;
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"Не удалось создать окно приложения.",
                    L"Tap Tap Dash PC", MB_OK | MB_ICONERROR);
        return 1;
    }

    constexpr int width = 770;
    constexpr int height = 590;
    RECT rect{0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    const int window_width = rect.right - rect.left;
    const int window_height = rect.bottom - rect.top;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;

    HWND window = CreateWindowExW(
        0, kClassName, L"Tap Tap Dash PC Launcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, window_width, window_height, nullptr, nullptr, instance, nullptr);

    if (!window) {
        MessageBoxW(nullptr, L"Не удалось открыть лаунчер.",
                    L"Tap Tap Dash PC", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(window, show_command);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    DeleteObject(g_title_font);
    DeleteObject(g_subtitle_font);
    DeleteObject(g_body_font);
    DeleteObject(g_small_font);
    DeleteObject(g_background_brush);
    return static_cast<int>(message.wParam);
}
