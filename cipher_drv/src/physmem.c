#include "../include/physmem.h"
#include "../include/log.h"

static BOOLEAN g_phys_ready = FALSE;

NTSTATUS physmem_init(void)
{
    g_phys_ready = TRUE;
    cipher_log("physmem ready");
    return STATUS_SUCCESS;
}

void physmem_shutdown(void)
{
    g_phys_ready = FALSE;
}

NTSTATUS physmem_va_to_pa(UINT64 cr3, UINT64 va, UINT64* out_pa)
{
    PHYSICAL_ADDRESS pa;

    if (!g_phys_ready || !out_pa)
        return STATUS_INVALID_PARAMETER;

    UNREFERENCED_PARAMETER(cr3);
    pa = MmGetPhysicalAddress((PVOID)(ULONG_PTR)va);
    if (pa.QuadPart == 0)
        return STATUS_UNSUCCESSFUL;

    *out_pa = (UINT64)pa.QuadPart;
    return STATUS_SUCCESS;
}

NTSTATUS physmem_read_physical(UINT64 pa, void* buffer, SIZE_T size)
{
    PHYSICAL_ADDRESS phys;
    PVOID mapped;

    if (!g_phys_ready || !buffer || size == 0)
        return STATUS_INVALID_PARAMETER;

    phys.QuadPart = (LONGLONG)pa;
    mapped = MmMapIoSpace(phys, size, MmNonCached);
    if (!mapped)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(buffer, mapped, size);
    MmUnmapIoSpace(mapped, size);
    return STATUS_SUCCESS;
}

NTSTATUS physmem_write_physical(UINT64 pa, const void* buffer, SIZE_T size)
{
    PHYSICAL_ADDRESS phys;
    PVOID mapped;

    if (!g_phys_ready || !buffer || size == 0)
        return STATUS_INVALID_PARAMETER;

    phys.QuadPart = (LONGLONG)pa;
    mapped = MmMapIoSpace(phys, size, MmNonCached);
    if (!mapped)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(mapped, buffer, size);
    MmUnmapIoSpace(mapped, size);
    return STATUS_SUCCESS;
}

NTSTATUS physmem_read_virtual(UINT64 cr3, UINT64 va, void* buffer, SIZE_T size)
{
    UINT64 pa = 0;
    NTSTATUS st = physmem_va_to_pa(cr3, va, &pa);
    if (!NT_SUCCESS(st))
        return st;
    return physmem_read_physical(pa, buffer, size);
}

NTSTATUS physmem_write_virtual(UINT64 cr3, UINT64 va, const void* buffer, SIZE_T size)
{
    UINT64 pa = 0;
    NTSTATUS st = physmem_va_to_pa(cr3, va, &pa);
    if (!NT_SUCCESS(st))
        return st;
    return physmem_write_physical(pa, buffer, size);
}
