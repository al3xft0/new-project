#include "../include/comm.h"
#include "../include/log.h"

// Phase 2 communication: named section + event.
// Usermode opens \BaseNamedObjects\CipherCommSection and CipherCommEvent.
// Layout of the section:
//   [cipher_request][data...][cipher_response][data...]
// Usermode writes request, signals event; driver processes, writes response, signals back.

#define SECTION_SIZE      (64 * 1024)
#define SECTION_NAME      L"\\BaseNamedObjects\\CipherCommSection"
#define EVENT_NAME        L"\\BaseNamedObjects\\CipherCommEvent"

static HANDLE            g_section_handle = NULL;
static PVOID             g_section_view   = NULL;
static HANDLE            g_event_handle   = NULL;
static HANDLE            g_thread_handle  = NULL;
static BOOLEAN           g_running        = FALSE;
static comm_handler_fn   g_handler        = NULL;

void comm_set_handler(comm_handler_fn handler)
{
    g_handler = handler;
}

static void process_request(void)
{
    if (!g_section_view || !g_handler)
        return;

    struct cipher_request*  req  = (struct cipher_request*)g_section_view;
    struct cipher_response* resp = (struct cipher_response*)((PUCHAR)g_section_view + 8192);
    void*                   data = (PUCHAR)g_section_view + 16384;

    if (req->magic != CIPHER_MAGIC || req->version != CIPHER_VERSION) {
        resp->magic   = CIPHER_MAGIC;
        resp->version = CIPHER_VERSION;
        resp->status  = ST_INVALID_PARAM;
        resp->size    = 0;
        return;
    }

    RtlZeroMemory(resp, sizeof(*resp));
    resp->magic   = CIPHER_MAGIC;
    resp->version = CIPHER_VERSION;

    g_handler(req, resp, data, SECTION_SIZE - 16384);
}

static void worker_thread(PVOID ctx)
{
    UNREFERENCED_PARAMETER(ctx);
    cipher_log("comm worker started");

    while (g_running) {
        LARGE_INTEGER timeout;
        timeout.QuadPart = -10 * 1000 * 1000; // 1s

        NTSTATUS st = KeWaitForSingleObject(
            g_event_handle, Executive, KernelMode, FALSE, &timeout);

        if (!g_running)
            break;

        if (st == STATUS_SUCCESS) {
            process_request();
            // re-signal so usermode knows response is ready (simple handshake)
            KeSetEvent((PRKEVENT)g_event_handle, IO_NO_INCREMENT, FALSE);
        }
    }

    cipher_log("comm worker exiting");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS comm_init(void)
{
    NTSTATUS status;
    UNICODE_STRING sectionName, eventName;
    OBJECT_ATTRIBUTES oa;
    LARGE_INTEGER maxSize;
    SIZE_T viewSize = SECTION_SIZE;

    RtlInitUnicodeString(&sectionName, SECTION_NAME);
    InitializeObjectAttributes(&oa, &sectionName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    maxSize.QuadPart = SECTION_SIZE;
    status = ZwCreateSection(&g_section_handle, SECTION_ALL_ACCESS, &oa,
                             &maxSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status)) {
        // already exists from previous run — open it
        status = ZwOpenSection(&g_section_handle, SECTION_ALL_ACCESS, &oa);
        if (!NT_SUCCESS(status)) {
            cipher_err("ZwCreateSection/Open failed 0x%X", status);
            return status;
        }
    }

    status = ZwMapViewOfSection(g_section_handle, ZwCurrentProcess(),
                                &g_section_view, 0, SECTION_SIZE, NULL,
                                &viewSize, ViewUnmap, 0, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        cipher_err("ZwMapViewOfSection failed 0x%X", status);
        ZwClose(g_section_handle);
        g_section_handle = NULL;
        return status;
    }

    RtlInitUnicodeString(&eventName, EVENT_NAME);
    InitializeObjectAttributes(&oa, &eventName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateEvent(&g_event_handle, EVENT_ALL_ACCESS, &oa, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        status = ZwOpenEvent(&g_event_handle, EVENT_ALL_ACCESS, &oa);
        if (!NT_SUCCESS(status)) {
            cipher_err("ZwCreateEvent/Open failed 0x%X", status);
            ZwUnmapViewOfSection(ZwCurrentProcess(), g_section_view);
            ZwClose(g_section_handle);
            g_section_view = NULL;
            g_section_handle = NULL;
            return status;
        }
    }

    g_running = TRUE;
    status = PsCreateSystemThread(&g_thread_handle, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                  worker_thread, NULL);
    if (!NT_SUCCESS(status)) {
        cipher_err("PsCreateSystemThread failed 0x%X", status);
        g_running = FALSE;
        ZwClose(g_event_handle);
        ZwUnmapViewOfSection(ZwCurrentProcess(), g_section_view);
        ZwClose(g_section_handle);
        g_event_handle = NULL;
        g_section_view = NULL;
        g_section_handle = NULL;
        return status;
    }

    cipher_log("comm ready (section + event)");
    return STATUS_SUCCESS;
}

void comm_shutdown(void)
{
    g_running = FALSE;

    if (g_event_handle) {
        KeSetEvent((PRKEVENT)g_event_handle, IO_NO_INCREMENT, FALSE);
    }

    if (g_thread_handle) {
        LARGE_INTEGER timeout;
        timeout.QuadPart = -5LL * 1000 * 1000 * 10; // 5s
        // best-effort wait; mapper unload will tear us down anyway
        ZwWaitForSingleObject(g_thread_handle, FALSE, &timeout);
        ZwClose(g_thread_handle);
        g_thread_handle = NULL;
    }

    if (g_section_view) {
        ZwUnmapViewOfSection(ZwCurrentProcess(), g_section_view);
        g_section_view = NULL;
    }
    if (g_section_handle) {
        ZwClose(g_section_handle);
        g_section_handle = NULL;
    }
    if (g_event_handle) {
        ZwClose(g_event_handle);
        g_event_handle = NULL;
    }

    cipher_log("comm shutdown");
}
