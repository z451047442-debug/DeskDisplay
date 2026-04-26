#include "pch.h"
#include "framework.h"
#include "DeskDisplay.h"
#include "SystemMonitor.h"
#include "OverlayRenderer.h"
#include "Config.h"
#include "Logger.h"
#include <commctrl.h>

constexpr int MAX_LOADSTRING = 100;
constexpr int WINDOW_WIDTH = 320;
constexpr int WINDOW_HEIGHT = 700;

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
ULONG_PTR gdiplusToken;

constexpr int IDM_TOGGLE_CLICKTHROUGH = 110;
constexpr int IDM_REFRESH_1000 = 111;
constexpr int IDM_REFRESH_2000 = 112;
constexpr int IDM_REFRESH_5000 = 113;
constexpr int IDM_REFRESH_500 = 115;
constexpr int IDM_SET_OPACITY = 114;
constexpr int HK_CLICKTHROUGH = 1;

struct AppState {
    SystemMonitor* monitor;
    OverlayRenderer* renderer;
    Config* config;
    bool clickThrough = false;
};

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int, SystemMonitor*, OverlayRenderer*, Config*);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK OpacityDlg(HWND, UINT, WPARAM, LPARAM);

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DESKDISPAY));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, SystemMonitor* monitor,
                   OverlayRenderer* renderer, Config* config) {
    hInst = hInstance;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int x = config->GetWindowX(screenW - WINDOW_WIDTH - 20);
    int posY = config->GetWindowY(50);
    int refreshMs = config->GetRefreshMs(2000);
    int opacity = config->GetOpacity(255);

    monitor->SetRefreshIntervalMs(refreshMs);
    renderer->SetOpacity(opacity);

    HWND hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        szWindowClass, szTitle,
        WS_POPUP,
        x, posY, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    auto* state = new AppState{ monitor, renderer, config };
    state->clickThrough = config->GetClickThrough(false);
    if (state->clickThrough) {
        LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
        SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
    }
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    SetTimer(hWnd, IDT_REFRESH, refreshMs, nullptr);
    if (!RegisterHotKey(hWnd, HK_CLICKTHROUGH, 0, VK_F8)) {
        Logger::Instance().Warn(L"RegisterHotKey F8 failed -- hotkey may be in use");
    }

    monitor->Collect();
    renderer->Render(hWnd, monitor->GetSysInfo(), monitor->GetNetStats(),
                     monitor->GetCpuHistory(), monitor->GetCpuMax(), monitor->GetCpuAvg(), 60);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool dragging = false;
    static POINT dragStart = {};

    switch (message) {
    case WM_LBUTTONDOWN:
        dragging = true;
        GetCursorPos(&dragStart);
        SetCapture(hWnd);
        break;
    case WM_MOUSEMOVE:
        if (dragging) {
            POINT pt;
            GetCursorPos(&pt);
            RECT wr;
            GetWindowRect(hWnd, &wr);
            MoveWindow(hWnd, wr.left + pt.x - dragStart.x, wr.top + pt.y - dragStart.y,
                       wr.right - wr.left, wr.bottom - wr.top, TRUE);
            dragStart = pt;
        }
        break;
    case WM_LBUTTONUP:
        dragging = false;
        ReleaseCapture();
        break;
    case WM_TIMER:
        if (wParam == IDT_REFRESH) {
            auto* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (state && state->monitor && state->renderer) {
                state->monitor->Collect();
                state->renderer->Render(hWnd, state->monitor->GetSysInfo(), state->monitor->GetNetStats(),
                                        state->monitor->GetCpuHistory(), state->monitor->GetCpuMax(),
                                        state->monitor->GetCpuAvg(), 60);
            }
        }
        break;
    case WM_RBUTTONUP: {
        auto* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (!state || !state->config) break;
        bool ct = state->clickThrough;
        int curRefresh = state->config->GetRefreshMs(2000);

        HMENU hMenu = CreatePopupMenu();
        HMENU hRefreshMenu = CreatePopupMenu();

        AppendMenuW(hRefreshMenu, MF_STRING | (curRefresh == 500 ? MF_CHECKED : 0),
                    IDM_REFRESH_500, L"实时");
        AppendMenuW(hRefreshMenu, MF_STRING | (curRefresh == 1000 ? MF_CHECKED : 0),
                    IDM_REFRESH_1000, L"1 秒");
        AppendMenuW(hRefreshMenu, MF_STRING | (curRefresh == 2000 ? MF_CHECKED : 0),
                    IDM_REFRESH_2000, L"2 秒");
        AppendMenuW(hRefreshMenu, MF_STRING | (curRefresh == 5000 ? MF_CHECKED : 0),
                    IDM_REFRESH_5000, L"5 秒");

        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hRefreshMenu),
                    L"刷新间隔(&R)");
        AppendMenuW(hMenu, MF_STRING, IDM_SET_OPACITY, L"设置透明度(&O)...");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING | (ct ? MF_CHECKED : 0), IDM_TOGGLE_CLICKTHROUGH,
                    L"点击穿透(&T)");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"关于(&A)...");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"退出(&X)");

        POINT pt;
        GetCursorPos(&pt);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
        DestroyMenu(hMenu);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDM_TOGGLE_CLICKTHROUGH: {
            auto* st = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (st) {
                st->clickThrough = !st->clickThrough;
                LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
                if (st->clickThrough) {
                    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
                } else {
                    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
                }
                if (st->config) st->config->SetClickThrough(st->clickThrough);
            }
            break;
        }
        case IDM_REFRESH_500:
        case IDM_REFRESH_1000:
        case IDM_REFRESH_2000:
        case IDM_REFRESH_5000: {
            auto* st = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (st && st->config && st->monitor) {
                int newMs = (wmId == IDM_REFRESH_500) ? 500 :
                            (wmId == IDM_REFRESH_1000) ? 1000 :
                            (wmId == IDM_REFRESH_2000) ? 2000 : 5000;
                KillTimer(hWnd, IDT_REFRESH);
                SetTimer(hWnd, IDT_REFRESH, newMs, nullptr);
                st->monitor->SetRefreshIntervalMs(newMs);
                st->config->SetRefreshMs(newMs);
            }
            break;
        }
        case IDM_SET_OPACITY: {
            auto* st = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (st && st->config && st->renderer) {
                int curOpacity = st->config->GetOpacity(255);
                int pct = (curOpacity * 100) / 255;
                if (pct < 10) pct = 10;
                INT_PTR result = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_OPACITYBOX), hWnd,
                                                OpacityDlg, reinterpret_cast<LPARAM>(&pct));
                if (result == IDOK) {
                    int newOpacity = (pct * 255) / 100;
                    if (newOpacity < 26) newOpacity = 26;
                    st->renderer->SetOpacity(newOpacity);
                    st->config->SetOpacity(newOpacity);
                }
            }
            break;
        }
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_HOTKEY:
        if (wParam == HK_CLICKTHROUGH) {
            auto* st = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
            if (st) {
                st->clickThrough = !st->clickThrough;
                LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
                if (st->clickThrough) {
                    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
                } else {
                    SetWindowLongPtrW(hWnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
                }
                if (st->config) st->config->SetClickThrough(st->clickThrough);
            }
        }
        break;
    case WM_MOUSEWHEEL: {
        auto* st = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (st && st->config && st->renderer) {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            int curOpacity = st->config->GetOpacity(255);
            int newOpacity = curOpacity + delta * 13;  // ~5% per notch
            if (newOpacity < 26) newOpacity = 26;
            if (newOpacity > 255) newOpacity = 255;
            st->renderer->SetOpacity(newOpacity);
            st->config->SetOpacity(newOpacity);
        }
        break;
    }
    case WM_DESTROY: {
        UnregisterHotKey(hWnd, HK_CLICKTHROUGH);
        auto* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
        if (state && state->config) {
            RECT wr;
            if (GetWindowRect(hWnd, &wr)) {
                state->config->SetWindowPos(wr.left, wr.top);
            }
        }
        delete state;
        KillTimer(hWnd, IDT_REFRESH);
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK OpacityDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static int* pPct = nullptr;

    switch (message) {
    case WM_INITDIALOG: {
        pPct = reinterpret_cast<int*>(lParam);
        HWND hSlider = GetDlgItem(hDlg, IDC_OPACITY_SLIDER);
        SendMessageW(hSlider, TBM_SETRANGE, TRUE, MAKELONG(10, 100));
        SendMessageW(hSlider, TBM_SETPOS, TRUE, *pPct);
        WCHAR buf[16];
        swprintf_s(buf, L"%d%%", *pPct);
        SetDlgItemTextW(hDlg, IDC_OPACITY_PERCENT, buf);
        return (INT_PTR)TRUE;
    }
    case WM_HSCROLL: {
        HWND hSlider = GetDlgItem(hDlg, IDC_OPACITY_SLIDER);
        int pos = (int)SendMessageW(hSlider, TBM_GETPOS, 0, 0);
        WCHAR buf[16];
        swprintf_s(buf, L"%d%%", pos);
        SetDlgItemTextW(hDlg, IDC_OPACITY_PERCENT, buf);
        *pPct = pos;
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    InitCommonControls();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return FALSE;
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
        WSACleanup();
        return FALSE;
    }

    SystemMonitor monitor;
    OverlayRenderer renderer(WINDOW_WIDTH, WINDOW_HEIGHT);
    Config config;

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DESKDISPAY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow, &monitor, &renderer, &config)) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        WSACleanup();
        return FALSE;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    WSACleanup();
    return (int)msg.wParam;
}
