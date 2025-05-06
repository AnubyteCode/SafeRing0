#pragma once

#include <windows.h>
#include <stdint.h>    // for uint64_t

#ifdef SAFERING0DLL_EXPORTS
#define SAFERING0_API __declspec(dllexport)
#else
#define SAFERING0_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // IOCTL codes must match the driver
#define IOCTL_READ_MSR  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_READ_PCI  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

// MSR_REQUEST structure
    typedef struct MSR_REQUEST {
        DWORD    Index;   // MSR index
        uint64_t Value;   // MSR value (output)
    } MSR_REQUEST;

    // PCI_REQUEST structure (read-only 32-bit)
    typedef struct PCI_REQUEST {
        BYTE  Bus;
        BYTE  Device;
        BYTE  Function;
        BYTE  Offset;
        DWORD Data;      // 32-bit PCI config data read
    } PCI_REQUEST;

    // DLL-exported API
    SAFERING0_API BOOL     InitializeOls(void);
    SAFERING0_API BOOL     DeinitializeOls(void);
    SAFERING0_API BOOL     IsCpuid(void);
    SAFERING0_API BOOL     IsMsr(void);
    SAFERING0_API BOOL     IsPci(void);

    SAFERING0_API BOOL     Cpuid(DWORD idx, DWORD* eax, DWORD* ebx, DWORD* ecx, DWORD* edx);
    SAFERING0_API BOOL     Rdmsr(DWORD index, uint64_t* value);
    SAFERING0_API BOOL     ReadPciConfigByte(BYTE bus, BYTE dev, BYTE func, BYTE offs, BYTE* value);

#ifdef __cplusplus
}
#endif
