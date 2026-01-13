#pragma once
#include <windows.h>
#include <string>

namespace UI {
    // Control IDs
    constexpr int ID_EDIT_SYSINFO = 1001;
    constexpr int ID_EDIT_FILEPATH = 1002;
    constexpr int ID_BTN_SELECT_FILE = 1003;
    constexpr int ID_COMBO_T_VALUES = 1004;
    constexpr int ID_CHECK_RUN_ALL = 1005;
    constexpr int ID_EDIT_RESULTS = 1006;
    constexpr int ID_BTN_RUN_TESTS = 1007;
    constexpr int ID_BTN_CLEAR = 1008;
    constexpr int ID_BTN_OPEN_FOLDER = 1009;
    
    // Functions
    BOOL CreateControls(HWND hwndParent);
    void HandleResize(HWND hwnd, int width, int height);
    void HandleCommand(HWND hwnd, WORD id, WORD notifyCode, HWND controlHwnd);
    void PopulateComputerInfo(HWND hwnd);
    
    // File selection
    bool SelectInputFile(HWND hwndParent);
    const std::wstring& GetSelectedFilePath();
}