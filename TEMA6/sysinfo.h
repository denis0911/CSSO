#pragma once
#include <string>

namespace SysInfo {
    // Get the number of physical CPU cores
    int GetPhysicalCoreCount();

    // Get general system information
    std::wstring GetSystemInformation();

    // Get detailed NUMA information
    std::wstring GetNumaInformation();

    // Get CPU Sets information
    std::wstring GetCpuSetsInformation();
}