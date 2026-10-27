#include "../include/comm.h"
#include "../include/log.h"

#define SECTION_SIZE      (64 * 1024)
#define SECTION_NAME      L"\\BaseNamedObjects\\CipherCommSection"
#define EVENT_NAME        L"\\BaseNamedObjects\\CipherCommEvent"

static HANDLE            g_section_handle = NULL;
static PVOID             g_section_view   = NULL;
static PKEVENT           g_event_object   = NULL;
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
    struct cipher_request*  req;
    struct cipher_response* resp;
    void*                   data;

    if (!g_section_view || !g_handler)
        return;

    req  = (struct cipher_request*)g_section_view;
    resp = (struct cipher_response*)((PUCHAR)g_section_view + 0x2000);
    data = (PUCHAR)g_section_view + 0x4000;

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

    g_handler(req, resp, data, SECTION_SIZE - 0x4000);
}

static void worker_thread(PVOID ctx)
{
    LARGE_INTEGER timeout;

    UNREFERENCED_PARAMETER(ctx);
    cipher_log("comm worker started");

    timeout.QuadPart = -10LL * 1000 * 1000;

    while (g_running) {
        NTSTATUS st;

        if (!g_event_object)
            break;

        st = KeWaitForSingleObject(g_event_object, Executive, KernelMode, FALSE, &timeout);

        if (!g_running)
            break;

        if (st == STATUS_SUCCESS) {
            process_request();
            KeSetEvent(g_event_object, IO_NO_INCREMENT, FALSE);
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
    HANDLE threadHandle = NULL;

    RtlInitUnicodeString(&sectionName, SECTION_NAME);
    InitializeObjectAttributes(&oa, &sectionName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    maxSize.QuadPart = SECTION_SIZE;
    status = ZwCreateSection(&g_section_handle, SECTION_ALL_ACCESS, &oa,
                             &maxSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status)) {
        status = ZwOpenSection(&g_section_handle, SECTION_ALL_ACCESS, &oa);
        if (!NT_SUCCESS(status)) {
            cipher_err("section create/open failed 0x%X", status);
            return status;
        }
    }

    status = ZwMapViewOfSection(g_section_handle, ZwCurrentProcess(),
                                &g_section_view, 0, SECTION_SIZE, NULL,
                                &viewSize, ViewUnmap, 0, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        cipher_err("MapViewOfSection failed 0x%X", status);
        ZwClose(g_section_handle);
        g_section_handle = NULL;
        return status;
    }

    RtlInitUnicodeString(&eventName, EVENT_NAME);
    g_event_object = IoCreateNotificationEvent(&eventName, &g_event_handle);
    if (!g_event_object) {
        cipher_err("IoCreateNotificationEvent failed");
        ZwUnmapViewOfSection(ZwCurrentProcess(), g_section_view);
        ZwClose(g_section_handle);
        g_section_view = NULL;
        g_section_handle = NULL;
        return STATUS_UNSUCCESSFUL;
    }
    KeClearEvent(g_event_object);

    g_running = TRUE;
    status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                  worker_thread, NULL);
    if (!NT_SUCCESS(status)) {
        cipher_err("PsCreateSystemThread failed 0x%X", status);
        g_running = FALSE;
        if (g_event_handle) { ZwClose(g_event_handle); g_event_handle = NULL; }
        g_event_object = NULL;
        ZwUnmapViewOfSection(ZwCurrentProcess(), g_section_view);
        ZwClose(g_section_handle);
        g_section_view = NULL;
        g_section_handle = NULL;
        return status;
    }
    g_thread_handle = threadHandle;

    cipher_log("comm ready");
    return STATUS_SUCCESS;
}

void comm_shutdown(void)
{
    g_running = FALSE;

    if (g_event_object)
        KeSetEvent(g_event_object, IO_NO_INCREMENT, FALSE);

    if (g_thread_handle) {
        ZwClose(g_thread_handle);
        g_thread_handle = NULL;
    }

    if (g_event_handle) {
        ZwClose(g_event_handle);
        g_event_handle = NULL;
        g_event_object = NULL;
    }

    if (g_section_view) {
        ZwUnmapViewOfSection(ZwCurrentProcess(), g_section_view);
        g_section_view = NULL;
    }
    if (g_section_handle) {
        ZwClose(g_section_handle);
        g_section_handle = NULL;
    }

    cipher_log("comm shutdown");
}
