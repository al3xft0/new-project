#pragma once
#include <ntifs.h>

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS physmem_init(void);
void     physmem_shutdown(void);
NTSTATUS physmem_va_to_pa(UINT64 cr3, UINT64 va, UINT64* out_pa);
NTSTATUS physmem_read_physical(UINT64 pa, void* buffer, SIZE_T size);
NTSTATUS physmem_write_physical(UINT64 pa, const void* buffer, SIZE_T size);
NTSTATUS physmem_read_virtual(UINT64 cr3, UINT64 va, void* buffer, SIZE_T size);
NTSTATUS physmem_write_virtual(UINT64 cr3, UINT64 va, const void* buffer, SIZE_T size);

#ifdef __cplusplus
}
#endif
