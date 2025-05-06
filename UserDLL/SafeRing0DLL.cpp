// SafeRing0DLL.cpp
#include "SafeRing0API.h"
#include <windows.h>
#include <intrin.h>

static HANDLE hDrv = INVALID_HANDLE_VALUE;

extern "C" {

    BOOL InitializeOls(void) {
        hDrv = CreateFileW(L"\\\\.\\SafeRing0",
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0, NULL);
        return (hDrv != INVALID_HANDLE_VALUE);
    }

    BOOL DeinitializeOls(void) {
        if (hDrv != INVALID_HANDLE_VALUE) CloseHandle(hDrv);
        hDrv = INVALID_HANDLE_VALUE;
        return TRUE;
    }

    BOOL IsCpuid(void) { return TRUE; }
    BOOL IsMsr(void) { return TRUE; }
    BOOL IsPci(void) { return TRUE; }
    BOOL IsPmc(void) { return TRUE; }

    BOOL Cpuid(DWORD idx, DWORD* eax, DWORD* ebx, DWORD* ecx, DWORD* edx) {
        int regs[4];
        __cpuid(regs, idx);
        *eax = regs[0]; *ebx = regs[1]; *ecx = regs[2]; *edx = regs[3];
        return TRUE;
    }

    BOOL Rdmsr(DWORD index, uint64_t* value) {
        if (hDrv == INVALID_HANDLE_VALUE) return FALSE;
        MSR_REQUEST req = { index, 0 };
        DWORD ret;
        if (!DeviceIoControl(hDrv, IOCTL_READ_MSR, &req, sizeof(req), &req, sizeof(req), &ret, NULL))
            return FALSE;
        *value = req.Value;
        return TRUE;
    }

    BOOL RdmsrTx(DWORD index, uint64_t* value) {
        if (hDrv == INVALID_HANDLE_VALUE) return FALSE;
        MSR_REQUEST req = { index, 0 };
        DWORD ret;
        if (!DeviceIoControl(hDrv, IOCTL_READ_MSR_AFFINITY, &req, sizeof(req), &req, sizeof(req), &ret, NULL))
            return FALSE;
        *value = req.Value;
        return TRUE;
    }

    BOOL Rdpmc(DWORD counterIndex, uint64_t* value) {
        if (hDrv == INVALID_HANDLE_VALUE) return FALSE;
        PMC_REQUEST req = { counterIndex, 0 };
        DWORD ret;
        if (!DeviceIoControl(hDrv, IOCTL_READ_PMC, &req, sizeof(req), &req, sizeof(req), &ret, NULL))
            return FALSE;
        *value = req.Value;
        return TRUE;
    }

    BOOL ReadPciConfigByte(BYTE bus, BYTE dev, BYTE func, BYTE offs, BYTE* out) {
        if (hDrv == INVALID_HANDLE_VALUE) return FALSE;
        PCI_REQUEST req = { bus, dev, func, offs, 0 };
        DWORD ret;
        if (!DeviceIoControl(hDrv, IOCTL_READ_PCI, &req, sizeof(req), &req, sizeof(req), &ret, NULL))
            return FALSE;
        *out = (BYTE)(req.Data & 0xFF);
        return TRUE;
    }

    BOOL ReadPciConfigDword(BYTE bus, BYTE dev, BYTE func, BYTE offs, DWORD* out) {
        if (hDrv == INVALID_HANDLE_VALUE) return FALSE;
        PCI_REQUEST req = { bus, dev, func, offs, 0 };
        DWORD ret;
        if (!DeviceIoControl(hDrv, IOCTL_READ_PCI, &req, sizeof(req), &req, sizeof(req), &ret, NULL))
            return FALSE;
        *out = req.Data;
        return TRUE;
    }

    BOOL WritePciConfigDword(BYTE bus, BYTE dev, BYTE func, BYTE offs, DWORD value) {
        if (hDrv == INVALID_HANDLE_VALUE) return FALSE;
        PCI_REQUEST req = { bus, dev, func, offs, value };
        DWORD ret;
        return DeviceIoControl(hDrv, IOCTL_WRITE_PCI, &req, sizeof(req), NULL, 0, &ret, NULL);
    }

} // extern "C"
