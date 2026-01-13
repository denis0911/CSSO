#include <windows.h>
#include <string>
#include <CommCtrl.h>
#include "ui.h"
#include "orchestration.h"

#pragma comment(lib, "comctl32.lib")

#ifndef UNICODE
#error "Project must be compiled with UNICODE enabled"
#endif

#ifndef _UNICODE
#error "_UNICODE must be defined"
#endif

// Custom message for delayed system info population
#define WM_POPULATE_SYSINFO (WM_USER + 100)

std::wstring GetLastErrorMessage(DWORD err) {
    if (err == 0) return L"No error";

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer, 0, NULL
    );

    std::wstring message(messageBuffer, size);
    LocalFree(messageBuffer);

    // Trim trailing newlines
    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) {
        message.pop_back();
    }
    return message;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        if (!UI::CreateControls(hwnd)) {
            MessageBoxW(hwnd, L"Failed to create UI controls", L"Error", MB_OK | MB_ICONERROR);
            return -1;
        }
        // Schedule system info population after window is fully created
        PostMessageW(hwnd, WM_POPULATE_SYSINFO, 0, 0);
        return 0;

    case WM_POPULATE_SYSINFO:
        // Populate system information after window is shown
        UI::PopulateComputerInfo(hwnd);
        return 0;

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, RGB(0, 0, 0));
        SetBkColor(hdcEdit, RGB(255, 255, 255));
        static HBRUSH hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));
        return (LRESULT)hbrBkgnd;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkColor(hdcStatic, GetSysColor(COLOR_3DFACE));
        return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
    }

    case WM_SIZE:
        UI::HandleResize(hwnd, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND:
        UI::HandleCommand(hwnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
        return 0;

    case WM_ORCHESTRATION_LOG:
        UI::HandleOrchestrationLog(hwnd, lParam);
        return 0;

    case WM_ORCHESTRATION_COMPLETE:
        UI::HandleOrchestrationComplete(hwnd);
        return 0;

    case WM_DESTROY:
        UI::Cleanup();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES
    };

    if (!InitCommonControlsEx(&icex)) {
        MessageBoxW(NULL, L"Failed to initialize common controls", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Register window class
    const wchar_t CLASS_NAME[] = L"CollatzTestWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = CLASS_NAME;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        std::wstring msg = L"Window class registration failed:\n" + GetLastErrorMessage(GetLastError());
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create main window
    HWND hwnd = CreateWindowExW(
        0,                              // Extended styles
        CLASS_NAME,                     // Window class
        L"Collatz Sequence Performance Tests",  // Title
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position (x, y)
        1000, 700,                      // Size (width, height)
        NULL,                           // Parent window
        NULL,                           // Menu
        hInstance,                      // Instance handle
        NULL                            // Additional data
    );

    if (hwnd == NULL) {
        std::wstring msg = L"Window creation failed:\n" + GetLastErrorMessage(GetLastError());
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Show and update window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}