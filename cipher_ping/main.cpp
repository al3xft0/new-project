/* cipher_ping v2 — section + submit flag only (no event) */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "shared.h"

static const wchar_t* kNames[] = {
    L"Global\\CipherCommSection",
    L"CipherCommSection",
};

int main(void)
{
    HANDLE h = NULL;
    void* view = NULL;
    cipher_shared_header* hdr;
    cipher_request* req;
    cipher_response* resp;
    int i;

    printf("cipher_ping v2\n");

    for (i = 0; i < 2; i++) {
        h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kNames[i]);
        if (h) break;
    }
    if (!h) {
        printf("[!] OpenFileMapping failed (%lu) — is driver mapped?\n", GetLastError());
        return 1;
    }
    printf("[+] section open\n");

    view = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, SECTION_SIZE);
    if (!view) {
        printf("[!] MapViewOfFile failed (%lu)\n", GetLastError());
        CloseHandle(h);
        return 1;
    }

    hdr  = (cipher_shared_header*)((char*)view + SEC_OFF_HEADER);
    req  = (cipher_request*)((char*)view + SEC_OFF_REQUEST);
    resp = (cipher_response*)((char*)view + SEC_OFF_RESPONSE);

    if (hdr->ready_magic != CIPHER_READY_MAGIC) {
        printf("[!] not ready (magic=0x%08X) — wrong build or not init\n", hdr->ready_magic);
        UnmapViewOfFile(view);
        CloseHandle(h);
        return 1;
    }
    printf("[+] ready magic ok (v%u)\n", hdr->version);

    memset(req, 0, sizeof(*req));
    memset(resp, 0, sizeof(*resp));
    req->magic   = CIPHER_MAGIC;
    req->version = CIPHER_VERSION;
    req->cmd     = CMD_PING;

    printf("[*] submit ping...\n");
    InterlockedExchange((volatile LONG*)&hdr->submit, 1);

    for (i = 0; i < 100; i++) {
        Sleep(20);
        if (hdr->submit == 0 && resp->magic == CIPHER_MAGIC)
            break;
    }

    if (hdr->submit != 0 || resp->magic != CIPHER_MAGIC) {
        printf("[!] timeout (submit=%u magic=0x%08X)\n", hdr->submit, resp->magic);
        UnmapViewOfFile(view);
        CloseHandle(h);
        return 1;
    }

    printf("[+] status=%d\n", resp->status);
    if (resp->status == ST_OK)
        printf("*** PING OK — channel live ***\n");
    else
        printf("*** ping non-OK ***\n");

    UnmapViewOfFile(view);
    CloseHandle(h);
    return resp->status == ST_OK ? 0 : 2;
}
