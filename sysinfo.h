#pragma once
#include <string>

namespace SysInfo {
    std::wstring GetSystemInformation();
    std::wstring GetCPUInfo();
    std::wstring GetMemoryInfo();
    std::wstring GetOSInfo();
}