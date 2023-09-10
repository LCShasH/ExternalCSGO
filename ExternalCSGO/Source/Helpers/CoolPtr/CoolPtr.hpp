#pragma once

#include <cstdint>
#include "../MemoryManager/CMemoryManager.hpp"

class GeniusPtr {
    char m_pBuffer[0x1000];
    std::size_t m_uiSize = 0;
    std::size_t m_uiBase = 0;
public:
    GeniusPtr(std::uintptr_t uiBase, std::size_t uiSize) : m_uiSize(uiSize), m_uiBase(uiBase) {
        g_pMemoryManager->RPM(uiBase, m_pBuffer, uiSize);
    }

    bool IsValid() {
        return m_uiBase;
    }

    template<class T>
    T Get(std::uintptr_t uiOffset) {
        if (uiOffset > m_uiSize)
            return T{};

        return *(T*)((std::uintptr_t)m_pBuffer + uiOffset);
    }
};