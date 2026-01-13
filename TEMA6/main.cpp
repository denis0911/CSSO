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
#error "Project must be compiled with _UNICODE defined"
#endif
#define WM_POPULATE_SYSINFO (WM_USER + 500)

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

    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) {
        message.pop_back();
    }
    return message;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        if (!UI::CreateControls(hwnd)) {
            return -1;
        }
        // Post message to populate after window is shown
        PostMessageW(hwnd, WM_POPULATE_SYSINFO, 0, 0);
        return 0;

    case WM_POPULATE_SYSINFO:
        UI::PopulateComputerInfo(hwnd);
        return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, RGB(0, 0, 0));  // Black text
        SetBkColor(hdcEdit, RGB(255, 255, 255));  // White background
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_SIZE:
        UI::HandleResize(hwnd, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND:
        UI::HandleCommand(hwnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
        return 0;

    case WM_DESTROY:
        UI::Cleanup();
        PostQuitMessage(0);
        return 0;

    case WM_ORCHESTRATION_LOG:
        UI::HandleOrchestrationLog(hwnd, lParam);
        return 0;

    case WM_ORCHESTRATION_COMPLETE:
        UI::HandleOrchestrationComplete(hwnd);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    const wchar_t CLASS_NAME[] = L"CollatzTestWindow";

    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    if (!InitCommonControlsEx(&icex)) {
        MessageBoxW(NULL, L"Failed to initialize common controls", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        std::wstring msg = L"Failed to register window class:\n" + GetLastErrorMessage(GetLastError());
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Collatz Sequence Performance Tests",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        std::wstring msg = L"Failed to create window:\n" + GetLastErrorMessage(GetLastError());
        MessageBoxW(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}