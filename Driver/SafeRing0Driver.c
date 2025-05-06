// SafeRing0Driver.c (KMDF SYS)
// ─────────────────────────────────────────────────────────────

#include <ntddk.h>
#include <wdf.h>
#include <intrin.h>    // __readmsr, __writemsr, __readpmc, __outdword, __indword

#define IOCTL_READ_MSR           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_READ_PCI           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRITE_PCI          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_READ_PMC           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_READ_MSR_AFFINITY  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _MSR_REQUEST {
    ULONG Index;
    ULONGLONG Value;
} MSR_REQUEST, * PMSR_REQUEST;

typedef struct _PCI_REQUEST {
    UCHAR  Bus, Device, Function;
    UCHAR  Offset;
    ULONG  Data;
} PCI_REQUEST, * PPCI_REQUEST;

typedef struct _PMC_REQUEST {
    ULONG CounterIndex;
    ULONGLONG Value;
} PMC_REQUEST, * PPMC_REQUEST;

// Driver declarations
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);
    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
}

NTSTATUS
EvtDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);
    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);
    if (!NT_SUCCESS(status)) return status;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
    ioQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    return WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
}

VOID
EvtIoDeviceControl(
    WDFQUEUE   Queue,
    WDFREQUEST Request,
    size_t     OutputBufferLength,
    size_t     InputBufferLength,
    ULONG      IoControlCode
)
{
    UNREFERENCED_PARAMETER(Queue);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    size_t retlen = 0;

    // READ_MSR
    if (IoControlCode == IOCTL_READ_MSR &&
        InputBufferLength >= sizeof(MSR_REQUEST) &&
        OutputBufferLength >= sizeof(MSR_REQUEST))
    {
        PMSR_REQUEST inReq = NULL, outReq = NULL;
        if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(MSR_REQUEST), (PVOID*)&inReq, NULL)) &&
            NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, sizeof(MSR_REQUEST), (PVOID*)&outReq, NULL)))
        {
            __try {
                outReq->Index = inReq->Index;
                outReq->Value = __readmsr(inReq->Index);
                status = STATUS_SUCCESS;
                retlen = sizeof(MSR_REQUEST);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                status = STATUS_ACCESS_VIOLATION;
            }
        }
    }
    // READ_MSR_AFFINITY
    else if (IoControlCode == IOCTL_READ_MSR_AFFINITY &&
        InputBufferLength >= sizeof(MSR_REQUEST) &&
        OutputBufferLength >= sizeof(MSR_REQUEST))
    {
        PMSR_REQUEST inReq = NULL, outReq = NULL;
        if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(MSR_REQUEST), (PVOID*)&inReq, NULL)) &&
            NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, sizeof(MSR_REQUEST), (PVOID*)&outReq, NULL)))
        {
            KAFFINITY mask = (KAFFINITY)1 << inReq->Index;
            KAFFINITY oldAff = KeSetSystemAffinityThreadEx(mask);

            __try {
                outReq->Index = inReq->Index;
                outReq->Value = __readmsr(inReq->Index);
                status = STATUS_SUCCESS;
                retlen = sizeof(MSR_REQUEST);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                status = STATUS_ACCESS_VIOLATION;
            }

            KeRevertToUserAffinityThreadEx(oldAff);
        }
        else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    // READ_PCI
    else if (IoControlCode == IOCTL_READ_PCI &&
        InputBufferLength >= sizeof(PCI_REQUEST) &&
        OutputBufferLength >= sizeof(PCI_REQUEST))
    {
        PPCI_REQUEST inReq = NULL, outReq = NULL;
        if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(PCI_REQUEST), (PVOID*)&inReq, NULL)) &&
            NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, sizeof(PCI_REQUEST), (PVOID*)&outReq, NULL)))
        {
            ULONG addr = (1u << 31) |
                (inReq->Bus << 16) |
                (inReq->Device << 11) |
                (inReq->Function << 8) |
                (inReq->Offset & 0xFC);
            __outdword(0xCF8, addr);
            outReq->Bus = inReq->Bus;
            outReq->Device = inReq->Device;
            outReq->Function = inReq->Function;
            outReq->Offset = inReq->Offset;
            outReq->Data = __indword(0xCFC);
            status = STATUS_SUCCESS;
            retlen = sizeof(PCI_REQUEST);
        }
    }
    // WRITE_PCI
    else if (IoControlCode == IOCTL_WRITE_PCI &&
        InputBufferLength >= sizeof(PCI_REQUEST))
    {
        PPCI_REQUEST inReq = NULL;
        if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(PCI_REQUEST), (PVOID*)&inReq, NULL)))
        {
            if (inReq->Bus == 0 && inReq->Device == 0 && inReq->Function == 0 && inReq->Offset == 0x60)
            {
                ULONG addr = (1u << 31) |
                    (inReq->Bus << 16) |
                    (inReq->Device << 11) |
                    (inReq->Function << 8) |
                    (inReq->Offset & 0xFC);
                __outdword(0xCF8, addr);
                __outdword(0xCFC, inReq->Data);
                status = STATUS_SUCCESS;
                retlen = sizeof(PCI_REQUEST);
            }
            else {
                status = STATUS_ACCESS_DENIED;
            }
        }
    }
    // READ_PMC
    else if (IoControlCode == IOCTL_READ_PMC &&
        InputBufferLength >= sizeof(PMC_REQUEST) &&
        OutputBufferLength >= sizeof(PMC_REQUEST))
    {
        PPMC_REQUEST inReq = NULL, outReq = NULL;
        if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(PMC_REQUEST), (PVOID*)&inReq, NULL)) &&
            NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, sizeof(PMC_REQUEST), (PVOID*)&outReq, NULL)))
        {
            __try {
                outReq->CounterIndex = inReq->CounterIndex;
                outReq->Value = __readpmc(inReq->CounterIndex);
                status = STATUS_SUCCESS;
                retlen = sizeof(PMC_REQUEST);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                status = STATUS_ACCESS_VIOLATION;
            }
        }
    }

    WdfRequestCompleteWithInformation(Request, status, retlen);
}
