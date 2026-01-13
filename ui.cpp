#include "ui.h"
#include "sysinfo.h"
#include "collatz.h"
#include "fileio.h"
#include "timing.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include <string>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// External declaration for GetLastErrorMessage from main.cpp
extern std::wstring GetLastErrorMessage(DWORD err);

namespace UI {
    // Control handles
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
    
    // File state
    static std::wstring g_selectedFilePath;
    
    BOOL CreateControls(HWND hwndParent) {
        // Create default font
        NONCLIENTMETRICSW ncm = { sizeof(ncm) };
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        hFont = CreateFontIndirectW(&ncm.lfMessageFont);
        
        int yPos = 10;
        
        // Computer Info Group
        hGroupSysInfo = CreateWindowExW(
            0, L"BUTTON", L"Computer Info",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            10, yPos, 960, 150,
            hwndParent, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hGroupSysInfo, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        hEditSysInfo = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            20, yPos + 25, 940, 115,
            hwndParent, (HMENU)ID_EDIT_SYSINFO, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hEditSysInfo, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        yPos += 170;
        
        // Input File Group
        hGroupInput = CreateWindowExW(
            0, L"BUTTON", L"Input File",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            10, yPos, 960, 70,
            hwndParent, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hGroupInput, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        hEditFilePath = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
            20, yPos + 25, 750, 25,
            hwndParent, (HMENU)ID_EDIT_FILEPATH, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hEditFilePath, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        hBtnSelectFile = CreateWindowExW(
            0, L"BUTTON", L"Select File...",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            780, yPos + 25, 170, 25,
            hwndParent, (HMENU)ID_BTN_SELECT_FILE, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hBtnSelectFile, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        yPos += 90;
        
        // Tests Group
        hGroupTests = CreateWindowExW(
            0, L"BUTTON", L"Tests",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            10, yPos, 960, 80,
            hwndParent, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hGroupTests, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        CreateWindowExW(
            0, L"STATIC", L"Number of iterations (T):",
            WS_CHILD | WS_VISIBLE,
            20, yPos + 27, 180, 20,
            hwndParent, NULL, GetModuleHandle(NULL), NULL
        );
        
        hComboT = CreateWindowExW(
            0, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            210, yPos + 25, 100, 200,
            hwndParent, (HMENU)ID_COMBO_T_VALUES, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hComboT, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Populate combo box
        const wchar_t* tValues[] = { L"5", L"10", L"50", L"100", L"500", L"1000", L"1500" };
        for (const auto& val : tValues) {
            SendMessageW(hComboT, CB_ADDSTRING, 0, (LPARAM)val);
        }
        SendMessageW(hComboT, CB_SETCURSEL, 0, 0);
        
        hCheckRunAll = CreateWindowExW(
            0, L"BUTTON", L"Run all T values",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            330, yPos + 27, 150, 20,
            hwndParent, (HMENU)ID_CHECK_RUN_ALL, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hCheckRunAll, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        yPos += 100;
        
        // Results Group
        hGroupResults = CreateWindowExW(
            0, L"BUTTON", L"Results",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            10, yPos, 960, 200,
            hwndParent, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hGroupResults, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        hEditResults = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            20, yPos + 25, 940, 155,
            hwndParent, (HMENU)ID_EDIT_RESULTS, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hEditResults, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        yPos += 220;
        
        // Action Buttons
        hBtnRunTests = CreateWindowExW(
            0, L"BUTTON", L"Run Tests",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, yPos, 150, 30,
            hwndParent, (HMENU)ID_BTN_RUN_TESTS, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hBtnRunTests, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        hBtnClear = CreateWindowExW(
            0, L"BUTTON", L"Clear",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            170, yPos, 150, 30,
            hwndParent, (HMENU)ID_BTN_CLEAR, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hBtnClear, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        hBtnOpenFolder = CreateWindowExW(
            0, L"BUTTON", L"Open Results Folder",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            330, yPos, 180, 30,
            hwndParent, (HMENU)ID_BTN_OPEN_FOLDER, GetModuleHandle(NULL), NULL
        );
        SendMessageW(hBtnOpenFolder, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        return TRUE;
    }
    
    void HandleResize(HWND hwnd, int width, int height) {
        // Basic layout recalculation on resize
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
        
        if (GetOpenFileNameW(&ofn) == TRUE) {
            // Verify file exists using GetFileAttributes
            DWORD fileAttr = GetFileAttributesW(szFile);
            
            if (fileAttr == INVALID_FILE_ATTRIBUTES) {
                DWORD err = GetLastError();
                std::wstring errorMsg = L"File verification failed:\n";
                errorMsg += GetLastErrorMessage(err);
                
                SetWindowTextW(hEditResults, errorMsg.c_str());
                MessageBoxW(hwndParent, errorMsg.c_str(), L"File Error", MB_OK | MB_ICONERROR);
                return false;
            }
            
            // Check if it's a directory
            if (fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring errorMsg = L"Selected path is a directory, not a file:\n";
                errorMsg += szFile;
                
                SetWindowTextW(hEditResults, errorMsg.c_str());
                MessageBoxW(hwndParent, errorMsg.c_str(), L"File Error", MB_OK | MB_ICONERROR);
                return false;
            }
            
            // File is valid - save path
            g_selectedFilePath = szFile;
            SetWindowTextW(hEditFilePath, g_selectedFilePath.c_str());
            
            // Try to map the file and display information
            FileIO::MappedFile mf;
            std::wstring err;
            
            if (FileIO::MapBinaryUInt32File(g_selectedFilePath, mf, err)) {
                std::wstringstream ss;
                ss << L"File mapped successfully:\r\n";
                ss << g_selectedFilePath << L"\r\n\r\n";
                ss << L"File size: " << mf.bytes << L" bytes\r\n";
                ss << L"Number count: " << mf.count << L" uint32_t values\r\n\r\n";
                
                // Show first few values as preview
                ss << L"Preview (first " << (mf.count < 10 ? mf.count : 10) << L" values):\r\n";
                for (size_t i = 0; i < mf.count && i < 10; i++) {
                    ss << L"  [" << i << L"] = " << mf.data[i] << L"\r\n";
                }
                
                if (mf.count > 10) {
                    ss << L"  ... (" << (mf.count - 10) << L" more values)\r\n";
                }
                
                SetWindowTextW(hEditResults, ss.str().c_str());
                
                // Clean up
                FileIO::UnmapFile(mf);
                return true;
            } else {
                // Error mapping file
                std::wstringstream ss;
                ss << L"Failed to map file:\r\n" << err;
                SetWindowTextW(hEditResults, ss.str().c_str());
                MessageBoxW(hwndParent, ss.str().c_str(), L"File Mapping Error", MB_OK | MB_ICONERROR);
                return false;
            }
        }
        else {
            // User cancelled or error occurred
            DWORD err = CommDlgExtendedError();
            if (err != 0) {
                // Dialog error occurred
                std::wstringstream ss;
                ss << L"File dialog error: 0x" << std::hex << err;
                SetWindowTextW(hEditResults, ss.str().c_str());
                MessageBoxW(hwndParent, ss.str().c_str(), L"Dialog Error", MB_OK | MB_ICONERROR);
            }
            return false;
        }
    }
    
    const std::wstring& GetSelectedFilePath() {
        return g_selectedFilePath;
    }
    
    void TestCollatzFunction(HWND hwnd) {
        std::wstringstream ss;
        ss << L"Collatz Function Test Results:\r\n";
        ss << L"================================\r\n\r\n";
        
        // Test cases
        struct TestCase {
            uint32_t input;
            uint32_t expected;
        };
        
        TestCase tests[] = {
            { 1, 0 },
            { 2, 1 },
            { 3, 7 },
            { 4, 2 },
            { 5, 5 },
            { 6, 8 },
            { 7, 16 },
            { 8, 3 },
            { 9, 19 },
            { 10, 6 },
            { 27, 111 },
            { 100, 25 },
            { 1000, 112 },
        };
        
        bool allPassed = true;
        for (const auto& test : tests) {
            uint32_t result = Collatz::CollatzSteps(test.input);
            bool passed = (result == test.expected);
            
            ss << L"CollatzSteps(" << test.input << L") = " << result;
            ss << L" (expected: " << test.expected << L")";
            ss << L" - " << (passed ? L"PASS" : L"FAIL") << L"\r\n";
            
            if (!passed) allPassed = false;
        }
        
        ss << L"\r\n================================\r\n";
        ss << L"Overall: " << (allPassed ? L"ALL TESTS PASSED ✓" : L"SOME TESTS FAILED ✗") << L"\r\n";
        
        SetWindowTextW(hEditResults, ss.str().c_str());
    }
    
    void HandleCommand(HWND hwnd, WORD id, WORD notifyCode, HWND controlHwnd) {
        switch (id) {
            case ID_BTN_SELECT_FILE:
                SelectInputFile(hwnd);
                break;
                
            case ID_BTN_RUN_TESTS:
                if (g_selectedFilePath.empty()) {
                    // If no file selected, run unit test instead
                    TestCollatzFunction(hwnd);
                } else {
                    MessageBoxW(hwnd, L"Test execution with file not yet implemented", L"Info", MB_OK | MB_ICONINFORMATION);
                }
                break;
                
            case ID_BTN_CLEAR:
                SetWindowTextW(hEditResults, L"");
                g_selectedFilePath.clear();
                SetWindowTextW(hEditFilePath, L"");
                break;
                
            case ID_BTN_OPEN_FOLDER:
                MessageBoxW(hwnd, L"Open folder not yet implemented", L"Info", MB_OK | MB_ICONINFORMATION);
                break;
        }
    }
    
    void PopulateComputerInfo(HWND hwnd) {
        std::wstring info = SysInfo::GetSystemInformation();
        SetWindowTextW(hEditSysInfo, info.c_str());
    }
}