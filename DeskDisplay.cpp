#include "pch.h"
#include "framework.h"
#include "DeskDisplay.h"
#include "SystemMonitor.h"
#include "OverlayRenderer.h"
#include "Config.h"
#include "Logger.h"

constexpr int MAX_LOADSTRING = 100;
constexpr int TIMER_INTERVAL = 2000;
constexpr int WINDOW_WIDTH = 320;
constexpr int WINDOW_HEIGHT = 700;

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
ULONG_PTR gdiplusToken;

constexpr int IDM_TOGGLE_CLICKTHROUGH = 110;
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

    HWND hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        szWindowClass, szTitle,
        WS_POPUP,
        x, posY, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    auto* state = new AppState{ monitor, renderer, config };
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    SetTimer(hWnd, IDT_REFRESH, TIMER_INTERVAL, nullptr);
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
        bool ct = state ? state->clickThrough : false;
        HMENU hMenu = CreatePopupMenu();
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
            }
        }
        break;
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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

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
