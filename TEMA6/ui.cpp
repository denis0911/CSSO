#include "ui.h"
#include "sysinfo.h"
#include "collatz.h"
#include "fileio.h"
#include "timing.h"
#include "security.h"
#include "sequential.h"
#include "parallel_static.h"
#include "parallel_dynamic.h"
#include "validation.h"
#include "orchestration.h"
#include <CommCtrl.h>
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

extern std::wstring GetLastErrorMessage(DWORD err);

namespace UI {
    static HWND hGroupSysInfo = NULL;
    static HWND hEditSysInfo = NULL;
    static HWND hGroupInput = NULL;
    static HWND hEditFilePath = NULL;
    static HWND hBtnSelectFile = NULL;
    static HWND hGroupTests = NULL;
    static HWND hComboT = NULL;
    static HWND hCheckRunAll = NULL;
    static HWND hGroupResults = NULL;
    static HWND hEditResults = NULL;
    static HWND hBtnRunTests = NULL;
    static HWND hBtnClear = NULL;
    static HWND hBtnOpenFolder = NULL;
    static HFONT hFont = NULL;
    static bool g_orchestrationRunning = false;
    static std::wstring g_selectedFilePath;

    // Helper function to force text visibility
    void ForceEditControlColors(HWND hEdit) {
        if (!hEdit || !IsWindow(hEdit)) return;

        HDC hdc = GetDC(hEdit);
        if (hdc) {
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkColor(hdc, RGB(255, 255, 255));
            ReleaseDC(hEdit, hdc);
        }

        InvalidateRect(hEdit, NULL, TRUE);
        UpdateWindow(hEdit);
    }

    // Helper function to set text with forced colors
    void SetEditText(HWND hEdit, const std::wstring& text) {
        if (!hEdit || !IsWindow(hEdit)) return;

        SetWindowTextW(hEdit, text.c_str());
        ForceEditControlColors(hEdit);
    }

    void LogError(const std::wstring& msg) {
        if (hEditResults) {
            int len = GetWindowTextLengthW(hEditResults);
            SendMessageW(hEditResults, EM_SETSEL, len, len);
            std::wstring errorMsg = L"ERROR: " + msg + L"\r\n";
            SendMessageW(hEditResults, EM_REPLACESEL, FALSE, (LPARAM)errorMsg.c_str());
            ForceEditControlColors(hEditResults);
        }
    }

    BOOL CreateControls(HWND hwndParent) {
        NONCLIENTMETRICSW ncm = { sizeof(ncm) };
        if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
            return FALSE;
        }

        hFont = CreateFontIndirectW(&ncm.lfMessageFont);
        if (!hFont) {
            return FALSE;
        }

        int yPos = 10;
        HINSTANCE hInst = GetModuleHandle(NULL);

        // Computer Info Group
        hGroupSysInfo = CreateWindowExW(0, L"BUTTON", L"Computer Info",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, yPos, 960, 150,
            hwndParent, NULL, hInst, NULL);
        if (!hGroupSysInfo) return FALSE;
        SendMessageW(hGroupSysInfo, WM_SETFONT, (WPARAM)hFont, TRUE);

        hEditSysInfo = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            20, yPos + 25, 940, 115,
            hwndParent, (HMENU)ID_EDIT_SYSINFO, hInst, NULL
        );
        if (!hEditSysInfo) return FALSE;
        SendMessageW(hEditSysInfo, WM_SETFONT, (WPARAM)hFont, TRUE);

        yPos += 170;

        // Input File Group
        hGroupInput = CreateWindowExW(0, L"BUTTON", L"Input File",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, yPos, 960, 70,
            hwndParent, NULL, hInst, NULL);
        if (!hGroupInput) return FALSE;
        SendMessageW(hGroupInput, WM_SETFONT, (WPARAM)hFont, TRUE);

        hEditFilePath = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
            20, yPos + 25, 750, 25,
            hwndParent, (HMENU)ID_EDIT_FILEPATH, hInst, NULL
        );
        if (!hEditFilePath) return FALSE;
        SendMessageW(hEditFilePath, WM_SETFONT, (WPARAM)hFont, TRUE);

        hBtnSelectFile = CreateWindowExW(0, L"BUTTON", L"Select File...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 780, yPos + 25, 170, 25,
            hwndParent, (HMENU)ID_BTN_SELECT_FILE, hInst, NULL);
        if (!hBtnSelectFile) return FALSE;
        SendMessageW(hBtnSelectFile, WM_SETFONT, (WPARAM)hFont, TRUE);

        yPos += 90;

        // Tests Group
        hGroupTests = CreateWindowExW(0, L"BUTTON", L"Tests",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, yPos, 960, 80,
            hwndParent, NULL, hInst, NULL);
        if (!hGroupTests) return FALSE;
        SendMessageW(hGroupTests, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hLabel = CreateWindowExW(0, L"STATIC", L"Number of iterations (T):",
            WS_CHILD | WS_VISIBLE, 20, yPos + 27, 180, 20, hwndParent, NULL, hInst, NULL);
        if (hLabel) SendMessageW(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        hComboT = CreateWindowExW(0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            210, yPos + 25, 100, 200, hwndParent, (HMENU)ID_COMBO_T_VALUES, hInst, NULL);
        if (!hComboT) return FALSE;
        SendMessageW(hComboT, WM_SETFONT, (WPARAM)hFont, TRUE);

        const wchar_t* tValues[] = { L"5", L"10", L"50", L"100", L"500", L"1000", L"1500" };
        for (const auto& val : tValues) {
            SendMessageW(hComboT, CB_ADDSTRING, 0, (LPARAM)val);
        }
        SendMessageW(hComboT, CB_SETCURSEL, 0, 0);

        hCheckRunAll = CreateWindowExW(0, L"BUTTON", L"Run all T values",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 330, yPos + 27, 150, 20,
            hwndParent, (HMENU)ID_CHECK_RUN_ALL, hInst, NULL);
        if (!hCheckRunAll) return FALSE;
        SendMessageW(hCheckRunAll, WM_SETFONT, (WPARAM)hFont, TRUE);

        yPos += 100;

        // Results Group
        hGroupResults = CreateWindowExW(0, L"BUTTON", L"Results",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 10, yPos, 960, 200,
            hwndParent, NULL, hInst, NULL);
        if (!hGroupResults) return FALSE;
        SendMessageW(hGroupResults, WM_SETFONT, (WPARAM)hFont, TRUE);

        hEditResults = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            20, yPos + 25, 940, 155,
            hwndParent, (HMENU)ID_EDIT_RESULTS, hInst, NULL
        );
        if (!hEditResults) return FALSE;
        SendMessageW(hEditResults, WM_SETFONT, (WPARAM)hFont, TRUE);

        yPos += 220;

        // Action Buttons
        hBtnRunTests = CreateWindowExW(0, L"BUTTON", L"Run Tests",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, yPos, 150, 30,
            hwndParent, (HMENU)ID_BTN_RUN_TESTS, hInst, NULL);
        if (!hBtnRunTests) return FALSE;
        SendMessageW(hBtnRunTests, WM_SETFONT, (WPARAM)hFont, TRUE);

        hBtnClear = CreateWindowExW(0, L"BUTTON", L"Clear",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 170, yPos, 150, 30,
            hwndParent, (HMENU)ID_BTN_CLEAR, hInst, NULL);
        if (!hBtnClear) return FALSE;
        SendMessageW(hBtnClear, WM_SETFONT, (WPARAM)hFont, TRUE);

        hBtnOpenFolder = CreateWindowExW(0, L"BUTTON", L"Open Results Folder",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, yPos, 180, 30,
            hwndParent, (HMENU)ID_BTN_OPEN_FOLDER, hInst, NULL);
        if (!hBtnOpenFolder) return FALSE;
        SendMessageW(hBtnOpenFolder, WM_SETFONT, (WPARAM)hFont, TRUE);

        std::wstring err;
        if (!FileIO::EnsureResultsFolders(err)) {
            LogError(L"Failed to create results folders: " + err);
        }

        return TRUE;
    }

    void PopulateComputerInfo(HWND hwnd) {
        std::wstringstream ss;

        // Basic system info
        ss << SysInfo::GetSystemInformation() << L"\r\n\r\n";

        // SID information (REQUIRED by 2.a)
        ss << L"========================================\r\n";
        ss << Security::GetSecurityInformation() << L"\r\n\r\n";

        // Detailed NUMA info (REQUIRED by 2.c)
        ss << L"========================================\r\n";
        ss << SysInfo::GetNumaInformation() << L"\r\n\r\n";

        // CPU Sets info (REQUIRED by 2.d)
        ss << L"========================================\r\n";
        ss << SysInfo::GetCpuSetsInformation();

        SetEditText(hEditSysInfo, ss.str());
    }

    void HandleResize(HWND hwnd, int width, int height) {
        InvalidateRect(hwnd, NULL, TRUE);
    }

    bool SelectInputFile(HWND hwndParent) {
        wchar_t szFile[MAX_PATH] = { 0 };

        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = hwndParent;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"Binary Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = L"Select Input File";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&ofn) != TRUE) {
            DWORD err = CommDlgExtendedError();
            if (err != 0) {
                std::wstringstream ss;
                ss << L"File dialog error: 0x" << std::hex << err;
                LogError(ss.str());
                MessageBoxW(hwndParent, ss.str().c_str(), L"Dialog Error", MB_OK | MB_ICONERROR);
            }
            return false;
        }

        DWORD fileAttr = GetFileAttributesW(szFile);
        if (fileAttr == INVALID_FILE_ATTRIBUTES) {
            std::wstring errorMsg = L"File verification failed: " + GetLastErrorMessage(GetLastError());
            LogError(errorMsg);
            MessageBoxW(hwndParent, errorMsg.c_str(), L"File Error", MB_OK | MB_ICONERROR);
            return false;
        }

        if (fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring errorMsg = L"Selected path is a directory, not a file: " + std::wstring(szFile);
            LogError(errorMsg);
            MessageBoxW(hwndParent, errorMsg.c_str(), L"File Error", MB_OK | MB_ICONERROR);
            return false;
        }

        g_selectedFilePath = szFile;
        SetEditText(hEditFilePath, g_selectedFilePath);

        FileIO::MappedFile mf;
        std::wstring err;

        if (!FileIO::MapBinaryUInt32File(g_selectedFilePath, mf, err)) {
            std::wstringstream ss;
            ss << L"Failed to map file:\r\n" << err;
            SetEditText(hEditResults, ss.str());
            MessageBoxW(hwndParent, ss.str().c_str(), L"File Mapping Error", MB_OK | MB_ICONERROR);
            return false;
        }

        std::wstringstream ss;
        ss << L"File mapped successfully:\r\n" << g_selectedFilePath << L"\r\n\r\n"
            << L"File size: " << mf.bytes << L" bytes\r\n"
            << L"Number count: " << mf.count << L" uint32_t values\r\n\r\n"
            << L"Preview (first " << (mf.count < 10 ? mf.count : 10) << L" values):\r\n";

        for (size_t i = 0; i < mf.count && i < 10; i++) {
            ss << L"  [" << i << L"] = " << mf.data[i] << L"\r\n";
        }

        if (mf.count > 10) {
            ss << L"  ... (" << (mf.count - 10) << L" more values)\r\n";
        }

        SetEditText(hEditResults, ss.str());
        FileIO::UnmapFile(mf);
        return true;
    }

    const std::wstring& GetSelectedFilePath() {
        return g_selectedFilePath;
    }

    void TestCollatzFunction(HWND hwnd) {
        std::wstringstream ss;
        ss << L"Collatz Function Test Results:\r\n================================\r\n\r\n";

        struct TestCase { uint32_t input; uint32_t expected; };
        TestCase tests[] = {
            { 1, 0 }, { 2, 1 }, { 3, 7 }, { 4, 2 }, { 5, 5 }, { 6, 8 },
            { 7, 16 }, { 8, 3 }, { 9, 19 }, { 10, 6 }, { 27, 111 }, { 100, 25 }, { 1000, 112 }
        };

        bool allPassed = true;
        for (const auto& test : tests) {
            uint32_t result = Collatz::CollatzSteps(test.input);
            bool passed = (result == test.expected);
            ss << L"CollatzSteps(" << test.input << L") = " << result
                << L" (expected: " << test.expected << L") - "
                << (passed ? L"PASS" : L"FAIL") << L"\r\n";
            if (!passed) allPassed = false;
        }

        ss << L"\r\n================================\r\n"
            << L"Overall: " << (allPassed ? L"ALL TESTS PASSED ✓" : L"SOME TESTS FAILED ✗") << L"\r\n";
        SetEditText(hEditResults, ss.str());
    }

    void TestTimingFunctions(HWND hwnd) {
        std::wstringstream ss;
        ss << L"High-Precision Timing Test Results:\r\n====================================\r\n\r\n";

        ss << L"Test 1: Sleep(10) - 10 milliseconds\r\n";
        LARGE_INTEGER start1 = Timing::NowQpc();
        Sleep(10);
        LARGE_INTEGER end1 = Timing::NowQpc();
        ss << L"  Measured: " << Timing::FormatMicros(Timing::ElapsedMicros(start1, end1)) << L"\r\n\r\n";

        ss << L"Test 2: Sleep(100) - 100 milliseconds\r\n";
        LARGE_INTEGER start2 = Timing::NowQpc();
        Sleep(100);
        LARGE_INTEGER end2 = Timing::NowQpc();
        ss << L"  Measured: " << Timing::FormatMicros(Timing::ElapsedMicros(start2, end2)) << L"\r\n\r\n";

        LARGE_INTEGER freq;
        if (QueryPerformanceFrequency(&freq)) {
            ss << L"System Performance Counter Info:\r\n  Frequency: " << freq.QuadPart << L" Hz\r\n";
        }

        ss << L"\r\n====================================\r\nAll timing tests completed!\r\n";
        SetEditText(hEditResults, ss.str());
    }

    void TestNumaInformation(HWND hwnd) {
        std::wstring numaInfo = SysInfo::GetNumaInformation();
        SetEditText(hEditResults, numaInfo);
    }

    void TestSecurityFunctions(HWND hwnd) {
        std::wstring secInfo = Security::GetSecurityInformation();
        SetEditText(hEditResults, secInfo);
    }

    void TestCpuSetsInformation(HWND hwnd) {
        std::wstring cpuSetsInfo = SysInfo::GetCpuSetsInformation();
        SetEditText(hEditResults, cpuSetsInfo);
    }

    void WriteSystemInfoFile(HWND hwnd) {
        std::wstringstream ss;
        ss << L"========================================\r\n"
            << L"SYSTEM INFORMATION REPORT\r\n"
            << L"========================================\r\n\r\n"
            << Security::GetSecurityInformation() << L"\r\n========================================\r\n\r\n"
            << SysInfo::GetSystemInformation() << L"\r\n========================================\r\n\r\n"
            << SysInfo::GetNumaInformation() << L"\r\n========================================\r\n\r\n"
            << SysInfo::GetCpuSetsInformation() << L"\r\n========================================\r\n";

        std::wstring err;
        if (FileIO::WriteSystemInfoToFile(ss.str(), err)) {
            std::wstringstream result;
            result << L"System information successfully written to:\r\nE:\\Facultate\\CSSO\\FinalWeek\\info.txt\r\n";
            SetEditText(hEditResults, result.str());
            MessageBoxW(hwnd, L"System information file created successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
        }
        else {
            LogError(L"Failed to write system info: " + err);
            MessageBoxW(hwnd, err.c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }

    void RunComprehensiveTests(HWND hwnd) {
        if (g_orchestrationRunning) {
            MessageBoxW(hwnd, L"Tests are already running!", L"Info", MB_OK | MB_ICONINFORMATION);
            return;
        }

        if (g_selectedFilePath.empty()) {
            MessageBoxW(hwnd, L"Please select an input file first!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        std::vector<uint32_t> selectedTs;
        BOOL runAll = (SendMessageW(hCheckRunAll, BM_GETCHECK, 0, 0) == BST_CHECKED);

        if (runAll) {
            selectedTs = { 5, 10, 50, 100, 500, 1000, 1500 };
        }
        else {
            int selIndex = static_cast<int>(SendMessageW(hComboT, CB_GETCURSEL, 0, 0));
            if (selIndex != CB_ERR) {
                wchar_t buffer[32];
                SendMessageW(hComboT, CB_GETLBTEXT, selIndex, reinterpret_cast<LPARAM>(buffer));
                selectedTs.push_back(_wtoi(buffer));
            }
            else {
                selectedTs.push_back(50);
            }
        }

        uint32_t P = Orchestration::GetPhysicalCoreCount();
        uint32_t maxWorkers = 2 * P;

        Orchestration::TestConfig config;
        config.inputFilePath = g_selectedFilePath;
        config.tValues = selectedTs;
        config.maxWorkers = maxWorkers;

        SetEditText(hEditResults, L"");
        g_orchestrationRunning = true;
        Orchestration::StartOrchestration(hwnd, config);
    }

    void HandleOrchestrationLog(HWND hwnd, LPARAM lParam) {
        wchar_t* message = reinterpret_cast<wchar_t*>(lParam);
        int len = GetWindowTextLengthW(hEditResults);
        SendMessageW(hEditResults, EM_SETSEL, len, len);
        SendMessageW(hEditResults, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(message));
        SendMessageW(hEditResults, EM_SCROLLCARET, 0, 0);
        ForceEditControlColors(hEditResults);
        delete[] message;
    }

    void HandleOrchestrationComplete(HWND hwnd) {
        g_orchestrationRunning = false;
        MessageBoxW(hwnd, L"Comprehensive test suite completed!", L"Complete", MB_OK | MB_ICONINFORMATION);
    }

    void HandleCommand(HWND hwnd, WORD id, WORD notifyCode, HWND controlHwnd) {
        switch (id) {
        case ID_BTN_SELECT_FILE:
            SelectInputFile(hwnd);
            break;

        case ID_BTN_RUN_TESTS:
            if (g_selectedFilePath.empty()) {
                HMENU hMenu = CreatePopupMenu();
                if (!hMenu) {
                    LogError(L"Failed to create menu: " + GetLastErrorMessage(GetLastError()));
                    break;
                }

                AppendMenuW(hMenu, MF_STRING, 1, L"Collatz Unit Tests");
                AppendMenuW(hMenu, MF_STRING, 2, L"Timing Tests");
                AppendMenuW(hMenu, MF_STRING, 3, L"Security/SID Tests");
                AppendMenuW(hMenu, MF_STRING, 4, L"NUMA Information");
                AppendMenuW(hMenu, MF_STRING, 5, L"CPU Sets Information");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, 6, L"Write System Info to File");

                POINT pt;
                GetCursorPos(&pt);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY,
                    pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);

                if (cmd == 1) TestCollatzFunction(hwnd);
                else if (cmd == 2) TestTimingFunctions(hwnd);
                else if (cmd == 3) TestSecurityFunctions(hwnd);
                else if (cmd == 4) TestNumaInformation(hwnd);
                else if (cmd == 5) TestCpuSetsInformation(hwnd);
                else if (cmd == 6) WriteSystemInfoFile(hwnd);
            }
            else {
                RunComprehensiveTests(hwnd);
            }
            break;

        case ID_BTN_CLEAR:
            SetEditText(hEditResults, L"");
            g_selectedFilePath.clear();
            SetEditText(hEditFilePath, L"");
            break;

        case ID_BTN_OPEN_FOLDER: {
            HINSTANCE result = ShellExecuteW(NULL, L"open",
                L"E:\\Facultate\\CSSO\\FinalWeek\\rezultate", NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)result <= 32) {
                LogError(L"Failed to open folder: " + GetLastErrorMessage(GetLastError()));
            }
            break;
        }
        }
    }

    void Cleanup() {
        if (hFont) {
            DeleteObject(hFont);
            hFont = NULL;
        }
    }
}