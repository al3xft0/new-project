/* cipher_drv minimal — DriverEntry only. No threads, no section, no event. */
#include <ntddk.h>

DRIVER_INITIALIZE DriverEntry;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
               "[cipher] minimal DriverEntry ok\n");

    return STATUS_SUCCESS;
}
