#include "../include/physmem.h"
#include "../include/log.h"

// Phase 2: minimal physical memory helpers.
// Uses MmMapIoSpace / MmCopyMemory style paths that work once the driver is mapped.
// Full page-table walk + CR3 decryption comes in a later hardening pass.

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

NTSTATUS physmem_va_to_pa(uint64_t cr3, uint64_t va, uint64_t* out_pa)
{
    if (!g_phys_ready || !out_pa)
        return STATUS_INVALID_PARAMETER;

    // Phase 2 simplification: for the system process / current context we can use
    // MmGetPhysicalAddress. Full CR3 walk is Phase 4.
    UNREFERENCED_PARAMETER(cr3);
    PHYSICAL_ADDRESS pa = MmGetPhysicalAddress((PVOID)va);
    if (pa.QuadPart == 0)
        return STATUS_UNSUCCESSFUL;
    *out_pa = (uint64_t)pa.QuadPart;
    return STATUS_SUCCESS;
}

NTSTATUS physmem_read_physical(uint64_t pa, void* buffer, size_t size)
{
    if (!g_phys_ready || !buffer || size == 0)
        return STATUS_INVALID_PARAMETER;

    PHYSICAL_ADDRESS phys;
    phys.QuadPart = (LONGLONG)pa;

    PVOID mapped = MmMapIoSpace(phys, size, MmNonCached);
    if (!mapped)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(buffer, mapped, size);
    MmUnmapIoSpace(mapped, size);
    return STATUS_SUCCESS;
}

NTSTATUS physmem_write_physical(uint64_t pa, const void* buffer, size_t size)
{
    if (!g_phys_ready || !buffer || size == 0)
        return STATUS_INVALID_PARAMETER;

    PHYSICAL_ADDRESS phys;
    phys.QuadPart = (LONGLONG)pa;

    PVOID mapped = MmMapIoSpace(phys, size, MmNonCached);
    if (!mapped)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(mapped, buffer, size);
    MmUnmapIoSpace(mapped, size);
    return STATUS_SUCCESS;
}

NTSTATUS physmem_read_virtual(uint64_t cr3, uint64_t va, void* buffer, size_t size)
{
    uint64_t pa = 0;
    NTSTATUS st = physmem_va_to_pa(cr3, va, &pa);
    if (!NT_SUCCESS(st))
        return st;
    return physmem_read_physical(pa, buffer, size);
}

NTSTATUS physmem_write_virtual(uint64_t cr3, uint64_t va, const void* buffer, size_t size)
{
    uint64_t pa = 0;
    NTSTATUS st = physmem_va_to_pa(cr3, va, &pa);
    if (!NT_SUCCESS(st))
        return st;
    return physmem_write_physical(pa, buffer, size);
}
