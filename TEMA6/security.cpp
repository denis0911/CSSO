#include "security.h"
#include <windows.h>
#include <sddl.h>
#include <aclapi.h>
#include <sstream>
#include <memory>

#pragma comment(lib, "advapi32.lib")

extern std::wstring GetLastErrorMessage(DWORD err);

namespace Security {
    std::wstring SidToString(PSID sid) {
        if (!sid || !IsValidSid(sid)) {
            return L"(Invalid SID)";
        }

        LPWSTR sidString = nullptr;
        if (ConvertSidToStringSidW(sid, &sidString)) {
            std::wstring result(sidString);
            LocalFree(sidString);
            return result;
        }

        return L"(Conversion failed)";
    }

    std::wstring GetCurrentUserSid() {
        HANDLE hToken = NULL;

        // Open the process token
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            DWORD err = GetLastError();
            return L"Error: " + GetLastErrorMessage(err);
        }

        // Get the size needed for TokenUser
        DWORD dwLength = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwLength);

        if (dwLength == 0) {
            CloseHandle(hToken);
            return L"Error: Failed to get token size";
        }

        // Allocate buffer and get token information
        std::unique_ptr<BYTE[]> buffer(new BYTE[dwLength]);
        PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.get());

        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwLength, &dwLength)) {
            DWORD err = GetLastError();
            CloseHandle(hToken);
            return L"Error: " + GetLastErrorMessage(err);
        }

        // Convert SID to string
        std::wstring sidString = SidToString(pTokenUser->User.Sid);

        CloseHandle(hToken);
        return sidString;
    }

    PSID GetCurrentUserSidBinary() {
        HANDLE hToken = NULL;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            return nullptr;
        }

        DWORD dwLength = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwLength);

        if (dwLength == 0) {
            CloseHandle(hToken);
            return nullptr;
        }

        BYTE* buffer = new BYTE[dwLength];
        PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer);

        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwLength, &dwLength)) {
            delete[] buffer;
            CloseHandle(hToken);
            return nullptr;
        }

        // Get the SID length and create a copy
        DWORD sidLength = GetLengthSid(pTokenUser->User.Sid);
        PSID pSidCopy = new BYTE[sidLength];

        if (!CopySid(sidLength, pSidCopy, pTokenUser->User.Sid)) {
            delete[] pSidCopy;
            delete[] buffer;
            CloseHandle(hToken);
            return nullptr;
        }

        delete[] buffer;
        CloseHandle(hToken);
        return pSidCopy;
    }

    std::wstring GetEveryoneSid() {
        // Calculate required size
        DWORD sidSize = SECURITY_MAX_SID_SIZE;
        std::unique_ptr<BYTE[]> sidBuffer(new BYTE[sidSize]);
        PSID pSid = reinterpret_cast<PSID>(sidBuffer.get());

        // Create the "Everyone" SID
        if (!CreateWellKnownSid(WinWorldSid, nullptr, pSid, &sidSize)) {
            DWORD err = GetLastError();
            return L"Error: " + GetLastErrorMessage(err);
        }

        return SidToString(pSid);
    }

    PSID GetEveryoneSidBinary() {
        DWORD sidSize = SECURITY_MAX_SID_SIZE;
        PSID pSid = new BYTE[sidSize];

        if (!CreateWellKnownSid(WinWorldSid, nullptr, pSid, &sidSize)) {
            delete[] pSid;
            return nullptr;
        }

        return pSid;
    }

    void FreeSidBinary(PSID sid) {
        if (sid) {
            delete[] reinterpret_cast<BYTE*>(sid);
        }
    }

    std::wstring GetAdministratorsSid() {
        // Calculate required size
        DWORD sidSize = SECURITY_MAX_SID_SIZE;
        std::unique_ptr<BYTE[]> sidBuffer(new BYTE[sidSize]);
        PSID pSid = reinterpret_cast<PSID>(sidBuffer.get());

        // Create the "Administrators" SID
        if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, pSid, &sidSize)) {
            DWORD err = GetLastError();
            return L"Error: " + GetLastErrorMessage(err);
        }

        return SidToString(pSid);
    }

    bool ApplyResultsFileAcl(const std::wstring& path, PSID currentUserSid, PSID everyoneSid, std::wstring& err) {
        err.clear();

        // Validate SIDs
        if (!currentUserSid || !IsValidSid(currentUserSid)) {
            err = L"Invalid current user SID";
            return false;
        }

        if (!everyoneSid || !IsValidSid(everyoneSid)) {
            err = L"Invalid Everyone SID";
            return false;
        }

        // Create EXPLICIT_ACCESS structures for ACL entries
        EXPLICIT_ACCESSW ea[2];
        ZeroMemory(&ea, sizeof(ea));

        // Entry 0: Current user - Read & Write access
        ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance = NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
        ea[0].Trustee.ptstrName = reinterpret_cast<LPWSTR>(currentUserSid);

        // Entry 1: Everyone - Read-only access
        ea[1].grfAccessPermissions = GENERIC_READ;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance = NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[1].Trustee.ptstrName = reinterpret_cast<LPWSTR>(everyoneSid);

        // Create new ACL from EXPLICIT_ACCESS structures
        PACL pNewAcl = nullptr;
        DWORD dwRes = SetEntriesInAclW(2, ea, nullptr, &pNewAcl);

        if (dwRes != ERROR_SUCCESS) {
            err = L"SetEntriesInAcl failed: " + GetLastErrorMessage(dwRes);
            return false;
        }

        // Apply the new ACL to the file
        dwRes = SetNamedSecurityInfoW(
            const_cast<LPWSTR>(path.c_str()),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
            nullptr,    // Owner
            nullptr,    // Group
            pNewAcl,    // DACL
            nullptr     // SACL
        );

        // Free the ACL (must be done regardless of success/failure)
        if (pNewAcl) {
            LocalFree(pNewAcl);
        }

        if (dwRes != ERROR_SUCCESS) {
            err = L"SetNamedSecurityInfo failed: " + GetLastErrorMessage(dwRes);
            return false;
        }

        return true;
    }

    std::wstring GetSecurityInformation() {
        std::wstringstream ss;

        ss << L"Security Information\r\n";
        ss << L"====================\r\n\r\n";

        // Current User SID
        ss << L"Current User SID:\r\n";
        std::wstring userSid = GetCurrentUserSid();
        ss << L"  " << userSid << L"\r\n\r\n";

        // Everyone SID
        ss << L"Everyone Group SID (WinWorldSid):\r\n";
        std::wstring everyoneSid = GetEveryoneSid();
        ss << L"  " << everyoneSid << L"\r\n\r\n";

        // Administrators SID
        ss << L"Administrators Group SID (WinBuiltinAdministratorsSid):\r\n";
        std::wstring adminsSid = GetAdministratorsSid();
        ss << L"  " << adminsSid << L"\r\n\r\n";

        // Additional information
        ss << L"SID Format Explanation:\r\n";
        ss << L"  S-1-5-21-X-X-X-RID  = Domain User\r\n";
        ss << L"  S-1-1-0             = Everyone\r\n";
        ss << L"  S-1-5-32-544        = BUILTIN\\Administrators\r\n\r\n";

        ss << L"These SIDs can be used for:\r\n";
        ss << L"  - Access Control Lists (ACLs)\r\n";
        ss << L"  - Security Descriptors\r\n";
        ss << L"  - File/Object permissions\r\n";
        ss << L"  - Token manipulation\r\n";

        return ss.str();
    }

    bool Security::ApplyResultsFileAcl(const std::wstring& filepath, std::wstring& err) {
        err.clear();

        PSID userSid = GetCurrentUserSidBinary();
        PSID everyoneSid = GetEveryoneSidBinary();

        if (!userSid || !everyoneSid) {
            if (userSid) FreeSidBinary(userSid);
            if (everyoneSid) FreeSidBinary(everyoneSid);
            err = L"Failed to get SIDs for ACL";
            return false;
        }

        bool ok = ApplyResultsFileAcl(filepath, userSid, everyoneSid, err);

        FreeSidBinary(userSid);
        FreeSidBinary(everyoneSid);
        return ok;
    }

}