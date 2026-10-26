#pragma once
#include <windows.h>

namespace StreamProof {
    const DWORD WDA_NONE = 0x0;
    const DWORD WDA_EXCLUDEFROMCAPTURE = 0x11;

    inline void Set(HWND hwnd, bool enabled) {
        SetWindowDisplayAffinity(hwnd, enabled ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
    }
}
