#include <ntifs.h>
#include "../include/physmem.h"
#include "../include/comm.h"
#include "../include/log.h"

void cipher_handle_request(const struct cipher_request* req,
                           struct cipher_response* resp,
                           void* data_buf, SIZE_T data_cap);

static BOOLEAN g_ready = FALSE;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    cipher_log("DriverEntry");

    status = physmem_init();
    if (!NT_SUCCESS(status)) {
        cipher_err("physmem_init failed 0x%X", status);
        return status;
    }

    comm_set_handler(cipher_handle_request);

    status = comm_init();
    if (!NT_SUCCESS(status)) {
        cipher_err("comm_init failed 0x%X", status);
        physmem_shutdown();
        return status;
    }

    g_ready = TRUE;
    cipher_log("cipher_drv ready");
    return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    if (!g_ready)
        return;
    g_ready = FALSE;
    comm_shutdown();
    physmem_shutdown();
    cipher_log("unloaded");
}
