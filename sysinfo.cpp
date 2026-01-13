#include "sysinfo.h"
#include <windows.h>
#include <sstream>
#include <iomanip>

namespace SysInfo {
    std::wstring GetCPUInfo() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        
        std::wstringstream ss;
        ss << L"Processors: " << sysInfo.dwNumberOfProcessors << L"\r\n";
        ss << L"Processor Architecture: ";
        
        switch (sysInfo.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_AMD64:
                ss << L"x64 (AMD or Intel)";
                break;
            case PROCESSOR_ARCHITECTURE_ARM:
                ss << L"ARM";
                break;
            case PROCESSOR_ARCHITECTURE_ARM64:
                ss << L"ARM64";
                break;
            case PROCESSOR_ARCHITECTURE_INTEL:
                ss << L"x86";
                break;
            default:
                ss << L"Unknown";
                break;
        }
        
        return ss.str();
    }
    
    std::wstring GetMemoryInfo() {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        GlobalMemoryStatusEx(&memStatus);
        
        std::wstringstream ss;
        ss << L"Total Physical Memory: " << (memStatus.ullTotalPhys / (1024 * 1024)) << L" MB\r\n";
        ss << L"Available Physical Memory: " << (memStatus.ullAvailPhys / (1024 * 1024)) << L" MB";
        
        return ss.str();
    }
    
    std::wstring GetOSInfo() {
        std::wstringstream ss;
        ss << L"Windows Version: " << GetVersion();
        return ss.str();
    }
    
    std::wstring GetSystemInformation() {
        std::wstringstream ss;

        // Get computer name
        wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if (GetComputerNameW(computerName, &size)) {
            ss << L"Computer Name: " << computerName << L"\r\n";
        }

        // Get system info
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);

        ss << L"Processor Architecture: ";
        switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            ss << L"x64 (AMD or Intel)";
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            ss << L"ARM";
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            ss << L"ARM64";
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            ss << L"x86";
            break;
        default:
            ss << L"Unknown";
            break;
        }
        ss << L"\r\n";

        ss << L"Number of Logical Processors: " << si.dwNumberOfProcessors << L"\r\n";

        // Get physical core count
        int physicalCores = GetPhysicalCoreCount();
        ss << L"Number of Physical Cores (P): " << physicalCores << L"\r\n";

        // Add NUMA info summary
        ULONG highestNodeNumber = 0;
        if (GetNumaHighestNodeNumber(&highestNodeNumber)) {
            ss << L"NUMA Nodes: " << (highestNodeNumber + 1) << L"\r\n";
        }

        ss << L"\r\nPhysical Core Details:\r\n";

        // Get processor information
        DWORD bufferSize = 0;
        GetLogicalProcessorInformation(nullptr, &bufferSize);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
                bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
            );

            if (GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
                size_t count = bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

                int coreIndex = 0;
                for (size_t i = 0; i < count; i++) {
                    if (buffer[i].Relationship == RelationProcessorCore) {
                        ss << L"  Core " << coreIndex << L": ";

                        // Count logical processors
                        int logicalCount = 0;
                        DWORD_PTR mask = buffer[i].ProcessorMask;
                        while (mask > 0) {
                            if (mask & 1) logicalCount++;
                            mask >>= 1;
                        }

                        ss << logicalCount << L" logical processor";
                        if (logicalCount > 1) ss << L"s";
                        ss << L" (Hyper-Threading ";
                        ss << (logicalCount > 1 ? L"enabled" : L"disabled");
                        ss << L")\r\n";

                        coreIndex++;
                    }
                }
            }
        }

        return ss.str();
    }
}