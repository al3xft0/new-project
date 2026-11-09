/*
 * cipher_drv channel v2
 * - Named section only (no IoCreateNotificationEvent)
 * - Worker polls header->submit with KeDelayExecutionThread
 * - __try/__except around request handling
 */
#include <ntddk.h>
#include "../include/shared.h"
#include "../include/log.h"

#define SECTION_NAME L"\\BaseNamedObjects\\CipherCommSection"

static HANDLE  g_section = NULL;
static PVOID   g_view    = NULL;
static HANDLE  g_thread  = NULL;
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

    if (hdr->submit != 1)
        return;

    __try {
        RtlZeroMemory(resp, sizeof(*resp));
        resp->magic   = CIPHER_MAGIC;
        resp->version = CIPHER_VERSION;

        if (req->magic != CIPHER_MAGIC || req->version != CIPHER_VERSION) {
            resp->status = ST_INVALID_PARAM;
        } else {
            switch (req->cmd) {
            case CMD_PING:
                resp->status = ST_OK;
                cipher_log("ping");
                break;
            default:
                resp->status = ST_INVALID_CMD;
                break;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        cipher_err("handler exception 0x%X", GetExceptionCode());
        resp->magic   = CIPHER_MAGIC;
        resp->version = CIPHER_VERSION;
        resp->status  = ST_INVALID_PARAM;
    }

    InterlockedExchange((volatile LONG*)&hdr->submit, 0);
}

static void worker(PVOID ctx)
{
    LARGE_INTEGER interval;
    UNREFERENCED_PARAMETER(ctx);

    cipher_log("worker start");
    /* 50ms poll */
    interval.QuadPart = -50LL * 10000LL;

    while (g_running) {
        handle_one();
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    cipher_log("worker exit");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS channel_init(void)
{
    NTSTATUS status;
    UNICODE_STRING name;
    OBJECT_ATTRIBUTES oa;
    LARGE_INTEGER maxSize;
    SIZE_T viewSize = SECTION_SIZE;
    struct cipher_shared_header* hdr;
    HANDLE th = NULL;

    RtlInitUnicodeString(&name, SECTION_NAME);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    maxSize.QuadPart = SECTION_SIZE;
    status = ZwCreateSection(&g_section, SECTION_ALL_ACCESS, &oa,
                             &maxSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status)) {
        status = ZwOpenSection(&g_section, SECTION_ALL_ACCESS, &oa);
        if (!NT_SUCCESS(status)) {
            cipher_err("section failed 0x%X", status);
            return status;
        }
    }

    status = ZwMapViewOfSection(g_section, ZwCurrentProcess(), &g_view,
                                0, SECTION_SIZE, NULL, &viewSize, ViewUnmap, 0, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        cipher_err("map view failed 0x%X", status);
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

    g_running = TRUE;
    status = PsCreateSystemThread(&th, THREAD_ALL_ACCESS, NULL, NULL, NULL, worker, NULL);
    if (!NT_SUCCESS(status)) {
        cipher_err("thread failed 0x%X", status);
        g_running = FALSE;
        ZwUnmapViewOfSection(ZwCurrentProcess(), g_view);
        ZwClose(g_section);
        g_view = NULL;
        g_section = NULL;
        return status;
    }
    g_thread = th;

    cipher_log("channel ready");
    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    cipher_log("DriverEntry");
    status = channel_init();
    if (!NT_SUCCESS(status)) {
        cipher_err("channel_init 0x%X", status);
        return status;
    }
    cipher_log("ready");
    return STATUS_SUCCESS;
}
