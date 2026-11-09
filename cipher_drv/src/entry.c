/*
 * cipher_drv commands v1
 * 1) Section + ready magic (proven stable)
 * 2) Worker starts only after 2s delay inside the thread
 * 3) Polls submit flag; handles CMD_PING only
 * 4) Thread create failure still returns SUCCESS (section stays usable)
 */
#include <ntddk.h>
#include "../include/shared.h"

#define SECTION_NAME L"\\BaseNamedObjects\\CipherCommSection"

static HANDLE g_section = NULL;
static PVOID  g_view    = NULL;
static HANDLE g_thread  = NULL;
static volatile BOOLEAN g_running = FALSE;

static void handle_one(void)
{
    struct cipher_shared_header* hdr;
    struct cipher_request* req;
    struct cipher_response* resp;

    if (!g_view)
        return;

    hdr  = (struct cipher_shared_header*)((PUCHAR)g_view + SEC_OFF_HEADER);
    req  = (struct cipher_request*)((PUCHAR)g_view + SEC_OFF_REQUEST);
    resp = (struct cipher_response*)((PUCHAR)g_view + SEC_OFF_RESPONSE);

    if (InterlockedCompareExchange((volatile LONG*)&hdr->submit, 0, 0) != 1)
        return;

    __try {
        RtlZeroMemory(resp, sizeof(*resp));
        resp->magic   = CIPHER_MAGIC;
        resp->version = CIPHER_VERSION;

        if (req->magic != CIPHER_MAGIC || req->version != CIPHER_VERSION) {
            resp->status = ST_INVALID_PARAM;
        } else if (req->cmd == CMD_PING) {
            resp->status = ST_OK;
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[cipher] ping\n");
        } else {
            resp->status = ST_INVALID_CMD;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        resp->magic   = CIPHER_MAGIC;
        resp->version = CIPHER_VERSION;
        resp->status  = ST_INVALID_PARAM;
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                   "[cipher] handler except 0x%X\n", GetExceptionCode());
    }

    InterlockedExchange((volatile LONG*)&hdr->submit, 0);
}

static void worker(PVOID ctx)
{
    LARGE_INTEGER delay;
    LARGE_INTEGER interval;

    UNREFERENCED_PARAMETER(ctx);

    /* Let DriverEntry finish and system settle before any work */
    delay.QuadPart = -2LL * 1000 * 1000 * 10; /* 2 seconds */
    KeDelayExecutionThread(KernelMode, FALSE, &delay);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[cipher] worker live\n");

    interval.QuadPart = -50LL * 10000LL; /* 50 ms */

    while (g_running) {
        handle_one();
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS setup_section(void)
{
    NTSTATUS status;
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES oa;
    LARGE_INTEGER maxSize;
    SIZE_T viewSize = SECTION_SIZE;
    struct cipher_shared_header* hdr;

    RtlInitUnicodeString(&name, SECTION_NAME);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    maxSize.QuadPart = SECTION_SIZE;
    status = ZwCreateSection(&g_section, SECTION_ALL_ACCESS, &oa,
                             &maxSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status)) {
        status = ZwOpenSection(&g_section, SECTION_ALL_ACCESS, &oa);
        if (!NT_SUCCESS(status))
            return status;
    }

    status = ZwMapViewOfSection(g_section, ZwCurrentProcess(), &g_view,
                                0, SECTION_SIZE, NULL, &viewSize, ViewUnmap, 0, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        ZwClose(g_section);
        g_section = NULL;
        return status;
    }

    RtlZeroMemory(g_view, SECTION_SIZE);
    hdr = (struct cipher_shared_header*)g_view;
    hdr->ready_magic = CIPHER_READY_MAGIC;
    hdr->version     = CIPHER_VERSION;
    hdr->submit      = 0;
    hdr->seq         = 0;
    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    HANDLE th = NULL;

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[cipher] commands v1 entry\n");

    status = setup_section();
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                   "[cipher] section failed 0x%X\n", status);
        return STATUS_SUCCESS; /* soft fail */
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[cipher] section ok\n");

    g_running = TRUE;
    status = PsCreateSystemThread(&th, THREAD_ALL_ACCESS, NULL, NULL, NULL, worker, NULL);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                   "[cipher] thread failed 0x%X — section only\n", status);
        g_running = FALSE;
        /* section still live for presence checks */
        return STATUS_SUCCESS;
    }
    g_thread = th;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[cipher] ready\n");
    return STATUS_SUCCESS;
}
