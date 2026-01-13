#include "sysinfo.h"
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <vector>

namespace SysInfo {
    int GetPhysicalCoreCount() {
        DWORD bufferSize = 0;

        // First call to get buffer size
        GetLogicalProcessorInformation(nullptr, &bufferSize);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return 0;
        }

        // Allocate buffer
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
            bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
        );

        // Second call to get actual data
        if (!GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
            return 0;
        }

        // Count physical cores
        int physicalCoreCount = 0;
        size_t count = bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

        for (size_t i = 0; i < count; i++) {
            if (buffer[i].Relationship == RelationProcessorCore) {
                physicalCoreCount++;
            }
        }

        return physicalCoreCount;
    }

    std::wstring GetCpuSetsInformation() {
        std::wstringstream ss;

        ss << L"CPU Sets Information:\r\n";
        ss << L"=====================\r\n\r\n";

        // === PART 1: Get Process Default CPU Sets ===
        ss << L"Process Default CPU Sets:\r\n";
        ss << L"-------------------------\r\n";

        HANDLE hProcess = GetCurrentProcess();
        ULONG cpuSetIdsCount = 0;
        ULONG requiredIdCount = 0;

        // First call to get the required size
        if (!GetProcessDefaultCpuSets(hProcess, nullptr, 0, &requiredIdCount)) {
            DWORD err = GetLastError();
            if (err == ERROR_INSUFFICIENT_BUFFER) {
                // Allocate buffer for CPU Set IDs
                std::vector<ULONG> cpuSetIds(requiredIdCount);

                // Second call to get actual data
                if (GetProcessDefaultCpuSets(hProcess, cpuSetIds.data(), requiredIdCount, &cpuSetIdsCount)) {
                    ss << L"Number of Process Default CPU Sets: " << cpuSetIdsCount << L"\r\n";

                    if (cpuSetIdsCount > 0) {
                        ss << L"CPU Set IDs:\r\n";
                        for (ULONG i = 0; i < cpuSetIdsCount; i++) {
                            ss << L"  [" << i << L"] = " << cpuSetIds[i] << L"\r\n";
                        }
                    }
                    else {
                        ss << L"  (Process uses all available CPU sets - no specific affinity)\r\n";
                    }
                }
                else {
                    ss << L"Failed to get process default CPU sets (Error: " << GetLastError() << L")\r\n";
                }
            }
            else {
                ss << L"Failed to query process default CPU sets size (Error: " << err << L")\r\n";
            }
        }
        else {
            // Success with zero buffer means no specific CPU sets assigned
            ss << L"Process has no specific CPU set assignment (uses all available)\r\n";
        }

        ss << L"\r\n";

        // === PART 2: Get System CPU Set Information ===
        ss << L"System CPU Set Information:\r\n";
        ss << L"---------------------------\r\n";

        ULONG returnLength = 0;
        ULONG requiredLength = 0;

        // First call to get required buffer size
        if (!GetSystemCpuSetInformation(nullptr, 0, &requiredLength, hProcess, 0)) {
            DWORD err = GetLastError();
            if (err == ERROR_INSUFFICIENT_BUFFER && requiredLength > 0) {
                // Allocate buffer
                std::vector<BYTE> buffer(requiredLength);
                PSYSTEM_CPU_SET_INFORMATION cpuSetInfo = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data());

                // Second call to get actual data
                if (GetSystemCpuSetInformation(cpuSetInfo, requiredLength, &returnLength, hProcess, 0)) {
                    ss << L"System CPU Set Information retrieved successfully\r\n";
                    ss << L"Buffer size: " << returnLength << L" bytes\r\n\r\n";

                    // Parse the returned structures
                    ULONG offset = 0;
                    int cpuSetIndex = 0;

                    while (offset < returnLength) {
                        PSYSTEM_CPU_SET_INFORMATION info = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data() + offset);

                        ss << L"CPU Set #" << cpuSetIndex << L":\r\n";
                        ss << L"  Size: " << info->Size << L" bytes\r\n";
                        ss << L"  Type: ";

                        if (info->Type == CpuSetInformation) {
                            ss << L"CpuSetInformation\r\n";

                            // Access the CpuSet structure
                            ss << L"  CPU Set ID: " << info->CpuSet.Id << L"\r\n";
                            ss << L"  Group: " << info->CpuSet.Group << L"\r\n";
                            ss << L"  Logical Processor Index: " << (ULONG)info->CpuSet.LogicalProcessorIndex << L"\r\n";
                            ss << L"  Core Index: " << (ULONG)info->CpuSet.CoreIndex << L"\r\n";
                            ss << L"  Last Level Cache Index: " << (ULONG)info->CpuSet.LastLevelCacheIndex << L"\r\n";
                            ss << L"  NUMA Node Index: " << (ULONG)info->CpuSet.NumaNodeIndex << L"\r\n";
                            ss << L"  Efficiency Class: " << (ULONG)info->CpuSet.EfficiencyClass << L"\r\n";

                            // Flags
                            ss << L"  Flags:\r\n";
                            if (info->CpuSet.Parked) ss << L"    - Parked\r\n";
                            if (info->CpuSet.Allocated) ss << L"    - Allocated\r\n";
                            if (info->CpuSet.AllocatedToTargetProcess) ss << L"    - Allocated to Target Process\r\n";
                            if (info->CpuSet.RealTime) ss << L"    - Real Time\r\n";

                            // Scheduling Class
                            ss << L"  Scheduling Class: ";
                            switch (info->CpuSet.SchedulingClass) {
                            case 0: ss << L"Standard\r\n"; break;
                            case 1: ss << L"SMT\r\n"; break;
                            default: ss << info->CpuSet.SchedulingClass << L"\r\n"; break;
                            }

                            // Allocation tag
                            ss << L"  Allocation Tag: 0x" << std::hex << std::uppercase
                                << std::setw(16) << std::setfill(L'0')
                                << info->CpuSet.AllocationTag << std::dec << L"\r\n";
                        }
                        else {
                            ss << L"Unknown (" << info->Type << L")\r\n";
                        }

                        ss << L"\r\n";

                        // Move to next structure
                        offset += info->Size;
                        cpuSetIndex++;
                    }

                    ss << L"Total CPU Sets found: " << cpuSetIndex << L"\r\n";
                }
                else {
                    ss << L"Failed to get system CPU set information (Error: " << GetLastError() << L")\r\n";
                }
            }
            else {
                ss << L"Failed to query system CPU set information size (Error: " << err << L")\r\n";
            }
        }
        else {
            ss << L"Unexpected success with zero buffer\r\n";
        }

        ss << L"\r\n";
        ss << L"Note: CPU Sets provide more granular control over processor\r\n";
        ss << L"affinity and allow better workload isolation on modern systems.\r\n";

        return ss.str();
    }

    std::wstring GetNumaInformation() {
        std::wstringstream ss;

        ss << L"NUMA (Non-Uniform Memory Access) Information:\r\n";
        ss << L"==============================================\r\n\r\n";

        // Get process and system affinity masks
        DWORD_PTR processAffinityMask = 0;
        DWORD_PTR systemAffinityMask = 0;

        if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask)) {
            ss << L"Process Affinity Mask: 0x" << std::hex << std::uppercase
                << std::setw(sizeof(DWORD_PTR) * 2) << std::setfill(L'0')
                << processAffinityMask << std::dec << L"\r\n";

            ss << L"System Affinity Mask:  0x" << std::hex << std::uppercase
                << std::setw(sizeof(DWORD_PTR) * 2) << std::setfill(L'0')
                << systemAffinityMask << std::dec << L"\r\n\r\n";

            // Count available processors
            int availableProcessors = 0;
            DWORD_PTR mask = systemAffinityMask;
            while (mask) {
                if (mask & 1) availableProcessors++;
                mask >>= 1;
            }
            ss << L"Available Processors (from system mask): " << availableProcessors << L"\r\n\r\n";
        }
        else {
            ss << L"Failed to get affinity masks.\r\n\r\n";
        }

        // Get NUMA node information
        ULONG highestNodeNumber = 0;
        if (GetNumaHighestNodeNumber(&highestNodeNumber)) {
            ss << L"NUMA Configuration:\r\n";
            ss << L"  Highest Node Number: " << highestNodeNumber << L"\r\n";
            ss << L"  Total NUMA Nodes: " << (highestNodeNumber + 1) << L"\r\n\r\n";

            // Get detailed information for each NUMA node
            ss << L"NUMA Node Details:\r\n";
            ss << L"------------------\r\n";

            for (ULONG node = 0; node <= highestNodeNumber; node++) {
                ss << L"NUMA Node " << node << L":\r\n";

                // Get processor mask for this NUMA node (Windows Vista+)
                GROUP_AFFINITY affinity;
                ZeroMemory(&affinity, sizeof(GROUP_AFFINITY));

                if (GetNumaNodeProcessorMaskEx((USHORT)node, &affinity)) {
                    ss << L"  Group: " << affinity.Group << L"\r\n";
                    ss << L"  Processor Mask: 0x" << std::hex << std::uppercase
                        << std::setw(sizeof(KAFFINITY) * 2) << std::setfill(L'0')
                        << affinity.Mask << std::dec << L"\r\n";

                    // Count processors in this node
                    int nodeProcessorCount = 0;
                    KAFFINITY nodeMask = affinity.Mask;
                    while (nodeMask) {
                        if (nodeMask & 1) nodeProcessorCount++;
                        nodeMask >>= 1;
                    }
                    ss << L"  Processors in Node: " << nodeProcessorCount << L"\r\n";
                }
                else {
                    ss << L"  Failed to get processor mask for this node.\r\n";
                }

                ss << L"\r\n";
            }

            // Add interpretation
            if (highestNodeNumber == 0) {
                ss << L"This system has a single NUMA node (UMA - Uniform Memory Access).\r\n";
            }
            else {
                ss << L"This system has multiple NUMA nodes.\r\n";
                ss << L"Memory access latency varies depending on the processor and memory location.\r\n";
            }
        }
        else {
            ss << L"NUMA information is not available on this system.\r\n";
            ss << L"This typically indicates a UMA (Uniform Memory Access) architecture.\r\n";
        }

        return ss.str();
    }

    std::wstring GetSystemInformation() {
        auto RelationshipToString = [](LOGICAL_PROCESSOR_RELATIONSHIP rel) -> const wchar_t* {
            switch (rel) {
            case RelationProcessorCore: return L"RelationProcessorCore";
            case RelationNumaNode: return L"RelationNumaNode";
            case RelationCache: return L"RelationCache";
            case RelationProcessorPackage: return L"RelationProcessorPackage";
            case RelationGroup: return L"RelationGroup";
            case RelationAll: return L"RelationAll";
            default: return L"RelationUnknown";
            }
            };

        std::wstringstream ss;

        // Get computer name
        wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if (GetComputerNameW(computerName, &size)) {
            ss << L"Computer Name: " << computerName << L"\r\n";
        }

        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);

        ss << L"Processor Architecture: ";
        switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: ss << L"x64 (AMD or Intel)"; break;
        case PROCESSOR_ARCHITECTURE_ARM: ss << L"ARM"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: ss << L"ARM64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: ss << L"x86"; break;
        default: ss << L"Unknown"; break;
        }
        ss << L"\r\n";

        ss << L"Number of Logical Processors: " << si.dwNumberOfProcessors << L"\r\n";

        int physicalCores = GetPhysicalCoreCount();
        ss << L"Number of Physical Cores (P): " << physicalCores << L"\r\n";

        ULONG highestNodeNumber = 0;
        if (GetNumaHighestNodeNumber(&highestNodeNumber)) {
            ss << L"NUMA Nodes: " << (highestNodeNumber + 1) << L"\r\n";
        }
        ss << L"\r\n";

        ss << L"HT API (GetLogicalProcessorInformation):\r\n";
        ss << L"----------------------------------------\r\n";

        DWORD bufferSize = 0;
        GetLogicalProcessorInformation(nullptr, &bufferSize);

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bufferSize == 0) {
            ss << L"GetLogicalProcessorInformation failed to query size. Error: " << GetLastError() << L"\r\n\r\n";
        }
        else {
            std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
                bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
            );

            if (!GetLogicalProcessorInformation(buffer.data(), &bufferSize)) {
                ss << L"GetLogicalProcessorInformation failed. Error: " << GetLastError() << L"\r\n\r\n";
            }
            else {
                const size_t count = bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                ss << L"Entries returned: " << count << L"\r\n";

                for (size_t i = 0; i < count; i++) {
                    ss << L"[" << i << L"] Relationship: " << RelationshipToString(buffer[i].Relationship) << L"\r\n";
                    ss << L"    ProcessorMask: 0x" << std::hex << std::uppercase
                        << std::setw(sizeof(ULONG_PTR) * 2) << std::setfill(L'0')
                        << buffer[i].ProcessorMask
                        << std::dec << L"\r\n";
                }

                ss << L"\r\n";
            }
        }

        ss << L"Physical Core Summary (from RelationProcessorCore):\r\n";
        ss << L"-----------------------------------------------\r\n";

        bufferSize = 0;
        GetLogicalProcessorInformation(nullptr, &bufferSize);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && bufferSize > 0) {
            std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer2(
                bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
            );
            if (GetLogicalProcessorInformation(buffer2.data(), &bufferSize)) {
                size_t count2 = bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
                int coreIndex = 0;

                for (size_t i = 0; i < count2; i++) {
                    if (buffer2[i].Relationship == RelationProcessorCore) {
                        ss << L"Physical Core " << coreIndex << L":\r\n";
                        ss << L"  ProcessorMask: 0x" << std::hex << std::uppercase
                            << std::setw(sizeof(ULONG_PTR) * 2) << std::setfill(L'0')
                            << buffer2[i].ProcessorMask << std::dec << L"\r\n";

                        int logicalCount = 0;
                        ULONG_PTR mask = buffer2[i].ProcessorMask;
                        while (mask) {
                            if (mask & 1) logicalCount++;
                            mask >>= 1;
                        }

                        ss << L"  Logical Processors: " << logicalCount;
                        if (logicalCount > 1) ss << L" (Hyper-Threading enabled)";
                        ss << L"\r\n";

                        coreIndex++;
                    }
                }
            }
        }

        ss << L"\r\n";

        // Memory
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            ss << L"Total Physical Memory: " << (memStatus.ullTotalPhys / (1024 * 1024)) << L" MB\r\n";
            ss << L"Available Physical Memory: " << (memStatus.ullAvailPhys / (1024 * 1024)) << L" MB\r\n";
            ss << L"Memory Load: " << memStatus.dwMemoryLoad << L"%\r\n";
        }

        ss << L"\r\nOperating System: Windows ";

        OSVERSIONINFOEXW osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

#pragma warning(push)
#pragma warning(disable: 4996)
        if (GetVersionExW((OSVERSIONINFOW*)&osvi)) {
            ss << osvi.dwMajorVersion << L"." << osvi.dwMinorVersion
                << L" Build " << osvi.dwBuildNumber;
        }
        else {
            ss << L"(Version detection not available)";
        }
#pragma warning(pop)

        return ss.str();
    }
}