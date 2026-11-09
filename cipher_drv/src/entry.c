/*
 * cipher_drv section-only
 * Creates named section, writes ready magic, returns.
 * NO thread. NO event. NO handler.
 */
#include <ntddk.h>
#include "../include/shared.h"

#define SECTION_NAME L"\\BaseNamedObjects\\CipherCommSection"

static HANDLE g_section = NULL;
static PVOID  g_view    = NULL;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES oa;
    LARGE_INTEGER maxSize;
    SIZE_T viewSize = SECTION_SIZE;
    struct cipher_shared_header* hdr;

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[cipher] section-only entry\n");

    RtlInitUnicodeString(&name, SECTION_NAME);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    maxSize.QuadPart = SECTION_SIZE;
    status = ZwCreateSection(&g_section, SECTION_ALL_ACCESS, &oa,
                             &maxSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status)) {
        status = ZwOpenSection(&g_section, SECTION_ALL_ACCESS, &oa);
        if (!NT_SUCCESS(status)) {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                       "[cipher] section failed 0x%X\n", status);
            return STATUS_SUCCESS; /* fail soft — don't BSOD the box */
        }
    }

    status = ZwMapViewOfSection(g_section, ZwCurrentProcess(), &g_view,
                                0, SECTION_SIZE, NULL, &viewSize, ViewUnmap, 0, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                   "[cipher] mapview failed 0x%X\n", status);
        if (g_section) ZwClose(g_section);
        g_section = NULL;
        return STATUS_SUCCESS; /* fail soft */
    }

    RtlZeroMemory(g_view, SECTION_SIZE);
    hdr = (struct cipher_shared_header*)g_view;
    hdr->ready_magic = CIPHER_READY_MAGIC;
    hdr->version     = CIPHER_VERSION;
    hdr->submit      = 0;
    hdr->seq         = 0;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[cipher] section ready\n");
    return STATUS_SUCCESS;
}
