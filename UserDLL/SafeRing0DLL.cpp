#include "SafeRing0API.h"
#include <windows.h>
#include <intrin.h>    // for __cpuid

static HANDLE hDrv = INVALID_HANDLE_VALUE;

extern "C" {

    BOOL InitializeOls(void) {
        hDrv = CreateFileW(L"\\\\.\\SafeRing0", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
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
        BOOL ok = DeviceIoControl(hDrv, IOCTL_READ_MSR, &req, sizeof(req), &req, sizeof(req), &ret, NULL);
        if (ok) *value = req.Value;
        return ok;
    }

    BOOL ReadPciConfigByte(BYTE bus, BYTE dev, BYTE func, BYTE offs, BYTE* out) {
        if (hDrv == INVALID_HANDLE_VALUE) return FALSE;
        PCI_REQUEST req = { bus, dev, func, offs, 0 };
        DWORD ret;
        BOOL ok = DeviceIoControl(hDrv, IOCTL_READ_PCI, &req, sizeof(req), &req, sizeof(req), &ret, NULL);
        if (ok) *out = (BYTE)(req.Data & 0xFF);
        return ok;
    }

} // extern "C"
