// SafeRing0API.h
// ─────────────────────────────────────────────────────────────
#pragma once

#include <windows.h>
#include <stdint.h>

#ifdef SAFERING0DLL_EXPORTS
#undef  SAFERING0_API
#define SAFERING0_API __declspec(dllexport)
#else
#undef  SAFERING0_API
#define SAFERING0_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // IOCTL codes must match the driver
#define IOCTL_READ_MSR           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_READ_PCI           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRITE_PCI          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_READ_PMC           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_READ_MSR_AFFINITY  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS)

// MSR request (read or affinity‐pinned read)
    typedef struct MSR_REQUEST {
        DWORD    Index;
        uint64_t Value;
    } MSR_REQUEST;

    // PCI request (read or write 32‐bit)
    typedef struct PCI_REQUEST {
        BYTE  Bus;
        BYTE  Device;
        BYTE  Function;
        BYTE  Offset;
        DWORD Data;    // if read: returned value; if write: value to write
    } PCI_REQUEST;

    // PMC request
    typedef struct PMC_REQUEST {
        DWORD   CounterIndex;
        uint64_t Value;
    } PMC_REQUEST;

    // DLL API
    SAFERING0_API BOOL InitializeOls(void);
    SAFERING0_API BOOL DeinitializeOls(void);

    SAFERING0_API BOOL IsCpuid(void);
    SAFERING0_API BOOL IsMsr(void);
    SAFERING0_API BOOL IsPci(void);
    SAFERING0_API BOOL IsPmc(void);

    SAFERING0_API BOOL Cpuid(DWORD idx, DWORD* eax, DWORD* ebx, DWORD* ecx, DWORD* edx);

    SAFERING0_API BOOL Rdmsr(DWORD index, uint64_t* value);
    SAFERING0_API BOOL RdmsrTx(DWORD index, uint64_t* value);
    SAFERING0_API BOOL Rdpmc(DWORD counterIndex, uint64_t* value);

    SAFERING0_API BOOL ReadPciConfigByte(BYTE bus, BYTE dev, BYTE func, BYTE offs, BYTE* value);
    SAFERING0_API BOOL ReadPciConfigDword(BYTE bus, BYTE dev, BYTE func, BYTE offs, DWORD* value);
    SAFERING0_API BOOL WritePciConfigDword(BYTE bus, BYTE dev, BYTE func, BYTE offs, DWORD value);

#ifdef __cplusplus
}
#endif
