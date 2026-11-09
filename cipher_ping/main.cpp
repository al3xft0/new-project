#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "shared.h"

int main(void)
{
    HANDLE h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"Global\\CipherCommSection");
    if (!h)
        h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"CipherCommSection");
    if (!h) {
        printf("[!] OpenFileMapping %lu\n", GetLastError());
        return 1;
    }

    void* view = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, SECTION_SIZE);
    if (!view) {
        printf("[!] MapViewOfFile %lu\n", GetLastError());
        CloseHandle(h);
        return 1;
    }

    cipher_shared_header* hdr = (cipher_shared_header*)((char*)view + SEC_OFF_HEADER);
    cipher_request* req = (cipher_request*)((char*)view + SEC_OFF_REQUEST);
    cipher_response* resp = (cipher_response*)((char*)view + SEC_OFF_RESPONSE);

    if (hdr->ready_magic != CIPHER_READY_MAGIC) {
        printf("[!] not ready magic=0x%08X\n", hdr->ready_magic);
        UnmapViewOfFile(view);
        CloseHandle(h);
        return 1;
    }
    printf("[+] ready (v%u) — waiting 3s for worker...\n", hdr->version);
    Sleep(3000);

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->magic = CIPHER_MAGIC;
    req->version = CIPHER_VERSION;
    req->cmd = CMD_PING;

    printf("[*] submit ping\n");
    InterlockedExchange((volatile LONG*)&hdr->submit, 1);

    for (int i = 0; i < 100; i++) {
        Sleep(20);
        if (hdr->submit == 0 && resp->magic == CIPHER_MAGIC)
            break;
    }

    if (hdr->submit != 0 || resp->magic != CIPHER_MAGIC) {
        printf("[!] timeout submit=%u magic=0x%08X\n", hdr->submit, resp->magic);
        UnmapViewOfFile(view);
        CloseHandle(h);
        return 1;
    }

    printf("[+] status=%d\n", resp->status);
    if (resp->status == ST_OK)
        printf("*** PING OK — commands live ***\n");
    else
        printf("*** non-OK ***\n");

    UnmapViewOfFile(view);
    CloseHandle(h);
    return resp->status == ST_OK ? 0 : 2;
}
