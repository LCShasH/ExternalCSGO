#include <iostream>
#include <Windows.h>
#include <vector>

#include "Helpers/MemoryManager/CMemoryManager.hpp"
#include "Helpers/Overlay/Overlay.h"
#include "Helpers/Math/Math.hpp"
#include "Helpers/CoolPtr/CoolPtr.hpp"

int main() {
    if (!g_pMemoryManager->Initialize()) {
        printf("[x] Failed to initialize memory system\n");
        return -1;
    }

    auto dwPid = g_pMemoryManager->GetGamePid();
    Overlay overlay(dwPid);

    HWND hWnd = FindWindowA(0, "Counter-Strike: Global Offensive - Direct3D 9");

    if(!hWnd) {
        printf("[x] Failed to get game window\n");
        return -1;
    }

    RECT rcClientRect;
    GetClientRect(hWnd, &rcClientRect);

    Math::g_Width = rcClientRect.right;
    Math::g_Height = rcClientRect.bottom;

    auto clientMod = g_pMemoryManager->GetClientInfo();
    auto engineMod = g_pMemoryManager->GetEngineInfo();

    auto clientBase = clientMod.dwBaseAddress;
    auto enigneBase = engineMod.dwBaseAddress;

    std::vector<int> entList;
    entList.resize(0x10 * 64);

    printf("[+] All is fine. Starting...\n");
    printf(" -- Panik key -> DELETE\n");

    while (!(GetAsyncKeyState(VK_DELETE) & 1)) {
        auto pLocalPlayer = g_pMemoryManager->RPM(clientBase + Offsets::LocalPlayer);

        if (!pLocalPlayer)
            continue;

        auto iLocalTeam = g_pMemoryManager->RPM(pLocalPlayer + Offsets::iTeamNum);

        g_pMemoryManager->RPM(clientBase + Offsets::EntityList, entList.data(), entList.size());

        VMatrix pViewMatrix = g_pMemoryManager->RPM< VMatrix >(clientBase + Offsets::ViewMatrix);

        if (!overlay.BeginFrame())
            continue;

        for (int i = 0; i < 64; i++) {
            auto pPlayer = GeniusPtr(entList.at(i * 4), 0x400);

            if (!pPlayer.IsValid())
                continue;

            auto bIsDormant = pPlayer.Get<bool>(Offsets::bDormant);

            if (bIsDormant)
                continue;
        
            auto iTeamNum = pPlayer.Get<std::int32_t>(Offsets::iTeamNum);

            if (iTeamNum == iLocalTeam)
                continue;

            auto iHealth = pPlayer.Get<std::uint32_t>(Offsets::iHealth);

            if (iHealth <= 0)
                continue;

            auto vecOrigin = pPlayer.Get< Vector >(Offsets::vecOrigin);

            auto vecMins = pPlayer.Get< Vector >( Offsets::vecMins);
            auto vecMaxs = pPlayer.Get< Vector >( Offsets::vecMaxs);

            Math::BBox_t bBox;
            if (Math::GetBoundingBox(vecOrigin, vecMins, vecMaxs, pViewMatrix, bBox)) {
                // ESP box
                overlay.DrawRect( bBox.x - 1, bBox.y - 1, bBox.w + 1, bBox.h + 1, 1, 0xFF000000);
                overlay.DrawRect (bBox.x, bBox.y, bBox.w, bBox.h, 1, 0xFFFFFFFF);
                overlay.DrawRect( bBox.x + 1, bBox.y + 1, bBox.w - 1, bBox.h - 1, 1, 0xFF000000);

                // Health bar
                // Bar
                const auto flHealthPercent = iHealth / 100.f;
                const auto flHeight = std::abs(bBox.h - bBox.y);

                // Premium skeet.cc healthbar ( why not :) )
                auto color = 0;
                if (iHealth >= 25) {
                    color = 0xA0D7C850;
                    if (iHealth >= 50)
                        color = 0xA050FF50;
                }
                else
                    color = 0xA0FF3250;

                overlay.DrawFilledRect(bBox.x - 5, bBox.y - 1, bBox.x - 3, bBox.h + 1, 0xFF000000);

                overlay.DrawFilledRect(bBox.x - 5, bBox.h - (flHeight * flHealthPercent), bBox.x - 3, bBox.h + 1, color);


                // Outlined rect
                overlay.DrawRect( bBox.x - 6, bBox.y - 1, bBox.x - 3, bBox.h + 1, 1, 0xFF000000);
            }
        }
        overlay.EndFrame();
    }

    return 0;
}