#pragma once
#include <string>
#include <windows.h>

namespace Security {
    // Convert a SID to string representation
    std::wstring SidToString(PSID sid);

    // Get the SID of the current user
    std::wstring GetCurrentUserSid();

    // Get the SID for the "Everyone" group
    std::wstring GetEveryoneSid();

    // Get the SID for the "Administrators" group
    std::wstring GetAdministratorsSid();

    // Get all security information formatted for display
    std::wstring GetSecurityInformation();

    // Apply ACL to result files (RW for current user, R for Everyone)
    bool ApplyResultsFileAcl(const std::wstring& path, PSID currentUserSid, PSID everyoneSid, std::wstring& err);

    // Helper to get current user SID as binary (for ACL operations)
    PSID GetCurrentUserSidBinary();

    // Helper to get Everyone SID as binary (for ACL operations)
    PSID GetEveryoneSidBinary();

    // Free SID allocated by helper functions
    void FreeSidBinary(PSID sid);
    bool ApplyResultsFileAcl(const std::wstring& filepath, std::wstring& err);
}
