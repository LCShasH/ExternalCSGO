#include <iostream>
#include <Windows.h>
#include <vector>

#include "Helpers/MemoryManager/CMemoryManager.hpp"
#include "Helpers/SharedMemoryStream/SharedMemoryStream.hpp"

struct vec3_t {
    float x, y, z;
};

struct RenderTextureRect_t
{
	DWORD renderCommand;
	int x0;
	int y0;
	int x1;
	int y1;
	float u0;
	float v0;
	float u1;
	float v1;
	float uk4;
	DWORD colorStart;
	DWORD colorEnd; // unused if gradient direction is set to none
	DWORD gradientDirection;
	DWORD textureId;
};

struct RenderTexture_t
{
    DWORD renderCommand;
    DWORD textureId;
    DWORD version;
    BOOL fullUpdate;
    DWORD size;
    DWORD width;
    DWORD height;
    DWORD x;
    DWORD y;
};

void DrawLine(int x1, int y1, int x2, int y2) {
	RenderTextureRect_t drawTexturedRect =
	{
		3, // render command
		0,
		0,
		400,
		150,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
		1.0f,
		0xFFFFFFFF,
		0xFFFFFFFF,
		3, // none
		1337
	};
}

int main() {
    g_pMemoryManager->Initialize();

    auto clientMod = g_pMemoryManager->GetClientInfo();

    auto clientBase = clientMod.dwBaseAddress;

    auto localPlayer = g_pMemoryManager->RPM(clientBase + Offsets::LocalPlayer);
    std::vector<int> entList;
    entList.resize(0x10 * 64);

    while (true) {
        g_pMemoryManager->RPM(clientBase + Offsets::EntityList, entList.data(), entList.size());

        auto iLocalTeam = g_pMemoryManager->RPM(localPlayer + Offsets::iTeamNum);

        for (int i = 0; i < 64; i++) {
            auto pPlayer = entList.at(i * 4);

            if (!pPlayer)
                continue;

            auto bIsDormant = g_pMemoryManager->RPM<bool>(pPlayer + Offsets::bDormant);

            if (bIsDormant)
                continue;
        
            auto iTeamNum = g_pMemoryManager->RPM(localPlayer + Offsets::iTeamNum);

            if (iTeamNum == iLocalTeam)
                continue;

            auto iHealth = g_pMemoryManager->RPM(localPlayer + Offsets::iHealth);



            printf("%i/64 -> %p health: %i\n", i, pPlayer, iHealth);
            
        }
    }

    return 0;
}