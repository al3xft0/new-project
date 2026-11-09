#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "shared.h"

int main(void)
{
    HANDLE h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"Global\\CipherCommSection");
    if (!h)
        h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"CipherCommSection");
    if (!h) {
        printf("[!] OpenFileMapping %lu — driver not mapped or section failed\n", GetLastError());
        return 1;
    }
    void* view = MapViewOfFile(h, FILE_MAP_READ, 0, 0, SECTION_SIZE);
    if (!view) {
        printf("[!] MapViewOfFile %lu\n", GetLastError());
        CloseHandle(h);
        return 1;
    }
    cipher_shared_header* hdr = (cipher_shared_header*)view;
    printf("ready_magic=0x%08X version=%u\n", hdr->ready_magic, hdr->version);
    if (hdr->ready_magic == CIPHER_READY_MAGIC)
        printf("*** SECTION OK — channel object live ***\n");
    else
        printf("*** bad magic ***\n");
    UnmapViewOfFile(view);
    CloseHandle(h);
    return 0;
}
