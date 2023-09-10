#include "CMemoryManager.hpp"

#include <TlHelp32.h>
#include <vector>

bool CMemoryManager::Initialize() {
    SearchGamePid("csgo.exe");

    if (!m_dwGamePid) {
        printf("[x] Failed to get game PID");
        return false;
    }

    m_hGameHandle = OpenProcess(0x1FFFFFu, 0, m_dwGamePid);

    if (m_hGameHandle == INVALID_HANDLE_VALUE) {
        printf("[x] Failed to open game process\n");
        return false;
    }

    m_ClientModule = GetProcessModuleInfo("client.dll");

    if (!m_ClientModule.dwBaseAddress) {
        printf("[x] Failed to get client.dll module\n");
        return false;
    }

    SetupOffsets();

    printf("[+] All is fine\n");
}

ModuleInfo_t CMemoryManager::GetProcessModuleInfo(const char* szModuleName) {

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_dwGamePid);
    DWORD dwModuleBaseAddress = 0;
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 ModuleEntry32 = { 0 };
        ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &ModuleEntry32))
        {
            do
            {
                if (strcmp(ModuleEntry32.szModule, szModuleName) == 0)
                {
                    dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &ModuleEntry32));
        }
        CloseHandle(hSnapshot);
    }

    char pBuffer[MAX_PATH + 1];
    K32GetModuleFileNameExA(m_hGameHandle, (HMODULE)dwModuleBaseAddress, pBuffer, MAX_PATH);

    return ModuleInfo_t{ dwModuleBaseAddress, pBuffer };
}

void CMemoryManager::SearchGamePid(const char* szProcessName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (strcmp(entry.szExeFile, szProcessName) == 0)
            {
                m_dwGamePid = entry.th32ProcessID;
                break;
            }
        }
    }

    CloseHandle(snapshot);
}

std::uintptr_t CMemoryManager::PatternScan(void* module, const char* signature)
{
    static auto pattern_to_byte = [](const char* pattern) {
        auto bytes = std::vector<int>{};
        auto start = const_cast<char*>(pattern);
        auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
        };

    auto dosHeader = (PIMAGE_DOS_HEADER)module;
    auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

    auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    auto patternBytes = pattern_to_byte(signature);
    auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

    auto s = patternBytes.size();
    auto d = patternBytes.data();

    for (auto i = 0ul; i < sizeOfImage - s; ++i) {
        bool found = true;
        for (auto j = 0ul; j < s; ++j) {
            if (scanBytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) {
            return (std::uintptr_t) & scanBytes[i];
        }
    }
    return 0;
}

std::uintptr_t GetDwordOffset(std::uintptr_t uiAddress, std::uintptr_t uiPreOffset, std::uintptr_t uiPostOffset, std::uintptr_t pBaseAddress) noexcept {
    return (*reinterpret_cast<std::int32_t*>(uiAddress + uiPreOffset) - uiPostOffset) - pBaseAddress;
}

void CMemoryManager::SetupOffsets() {
    // BB ? ? ? ? 83 FF 01 0F 8C ? ? ? ? 3B F8
    auto hClientDll = LoadLibraryExA(m_ClientModule.strModulePath.c_str(), 0, DONT_RESOLVE_DLL_REFERENCES);
    // EntityList
    // 0x4E0102C
    /*
    .text:102CF964 8B 42 18                                mov     eax, [edx+18h]
    .text:102CF967 3B C7                                   cmp     eax, edi
    .text:102CF969 0F 8C DD 02 00 00                       jl      loc_102CFC4C
    .text:102CF96F BB 2C 10 E0 14                          mov     ebx, offset unk_14E0102C <-- what we need
    */
    Offsets::EntityList = GetDwordOffset(PatternScan(hClientDll, "BB ? ? ? ? 83 FF 01 0F 8C ? ? ? ? 3B F8"), 1, 0, std::uintptr_t(hClientDll));

    // LocalPlayer
    // 0xDEB99C
    /*
    lea     eax, [edx+edx*4]
    .text:102B9097 42                                      inc     edx
    .text:102B9098 56                                      push    esi
    .text:102B9099 8D 34 85 98 B9 DE 10                    lea     esi, off_10DEB998[eax*4]
    */
    Offsets::LocalPlayer = GetDwordOffset(PatternScan(hClientDll, "8D 34 85 ? ? ? ? 89 15 ? ? ? ? 8B 41 08 8B 48 04 83 F9 FF"), 3, -4, std::uintptr_t(hClientDll));

    // ViewMatrix
    // 0x4DF1E74
    /*
    .text:1021F4F2 0F 84 C1 00 00 00                       jz      loc_1021F5B9
    .text:1021F4F8 0F 10 05 C4 1D DF 14                    movups  xmm0, xmmword_14DF1DC4 ; need + 
    .text:1021F4FF 8D 85 78 FE FF FF                       lea     eax, [ebp-188h]
    */

    Offsets::ViewMatrix = GetDwordOffset(PatternScan(hClientDll, "0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9"), 3, -0xB0, std::uintptr_t(hClientDll));

    
    //CloseHandle(hClientDll);
}
