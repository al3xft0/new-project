#include <ntddk.h>
#include "../include/physmem.h"
#include "../include/comm.h"
#include "../include/log.h"

// Forward from commands.c
void cipher_handle_request(const struct cipher_request* req,
                           struct cipher_response* resp,
                           void* data_buf, size_t data_cap);

static BOOLEAN g_ready = FALSE;

// Called by the mapper after the image is copied and relocs are applied.
// Signature matches what most manual mappers expect:
//   NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
// For pure manual map (MSRMapper / kdmapper style) both args may be NULL.
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    cipher_log("DriverEntry");

    NTSTATUS status = physmem_init();
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

// Optional clean unload if the mapper supports it later
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
