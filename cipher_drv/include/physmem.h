#pragma once
#include <ntddk.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS physmem_init(void);
void     physmem_shutdown(void);

// Translate VA under given CR3 to physical address
NTSTATUS physmem_va_to_pa(uint64_t cr3, uint64_t va, uint64_t* out_pa);

// Copy from physical address into kernel buffer
NTSTATUS physmem_read_physical(uint64_t pa, void* buffer, size_t size);

// Copy from kernel buffer to physical address
NTSTATUS physmem_write_physical(uint64_t pa, const void* buffer, size_t size);

// High-level: read/write virtual memory of a process given its CR3
NTSTATUS physmem_read_virtual(uint64_t cr3, uint64_t va, void* buffer, size_t size);
NTSTATUS physmem_write_virtual(uint64_t cr3, uint64_t va, const void* buffer, size_t size);

#ifdef __cplusplus
}
#endif
