#include <ntddk.h>
#include <wdf.h>
#include <intrin.h>    // for __readmsr, __outdword, __indword

#define IOCTL_READ_MSR  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_READ_PCI  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef struct _MSR_REQUEST {
    ULONG Index;
    ULONGLONG Value;
} MSR_REQUEST, * PMSR_REQUEST;

typedef struct _PCI_REQUEST {
    UCHAR Bus, Device, Function;
    UCHAR Offset;
    ULONG Data;    // 32-bit read
} PCI_REQUEST, * PPCI_REQUEST;

// Forward declarations of our event callbacks
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD     EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

// ----------------------------------------------------------------------------
// DriverEntry: standard KMDF entry point
// ----------------------------------------------------------------------------
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    // Create the framework driver object
    return WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );
}

// ----------------------------------------------------------------------------
// EvtDeviceAdd: called when PnP manager enumerates our device (Root\SafeRing0)
// ----------------------------------------------------------------------------
NTSTATUS
EvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG  ioQueueConfig;

    // Use buffered I/O
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    // Create the WDFDEVICE
    status = WdfDeviceCreate(
        &DeviceInit,
        WDF_NO_OBJECT_ATTRIBUTES,
        &device
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Configure a default I/O queue for DEVICE_CONTROL
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &ioQueueConfig,
        WdfIoQueueDispatchSequential
    );
    ioQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &ioQueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
    );
    return status;
}

// ----------------------------------------------------------------------------
// EvtIoDeviceControl: handle IOCTL_READ_MSR and IOCTL_READ_PCI
// ----------------------------------------------------------------------------
VOID
EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
)
{
    UNREFERENCED_PARAMETER(Queue);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    size_t   retlen = 0;

    if (IoControlCode == IOCTL_READ_MSR &&
        InputBufferLength >= sizeof(MSR_REQUEST) &&
        OutputBufferLength >= sizeof(MSR_REQUEST))
    {
        PMSR_REQUEST req;
        if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(*req), (PVOID*)&req, NULL)) &&
            NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, sizeof(*req), (PVOID*)&req, NULL)))
        {
            __try {
                req->Value = __readmsr(req->Index);
                status = STATUS_SUCCESS;
                retlen = sizeof(*req);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                status = STATUS_ACCESS_VIOLATION;
            }
        }
    }
    else if (IoControlCode == IOCTL_READ_PCI &&
        InputBufferLength >= sizeof(PCI_REQUEST) &&
        OutputBufferLength >= sizeof(PCI_REQUEST))
    {
        PPCI_REQUEST req;
        if (NT_SUCCESS(WdfRequestRetrieveInputBuffer(Request, sizeof(*req), (PVOID*)&req, NULL)) &&
            NT_SUCCESS(WdfRequestRetrieveOutputBuffer(Request, sizeof(*req), (PVOID*)&req, NULL)))
        {
            ULONG addr = (1u << 31) |
                (req->Bus << 16) |
                (req->Device << 11) |
                (req->Function << 8) |
                (req->Offset & 0xFC);
            __outdword(0xCF8, addr);
            req->Data = __indword(0xCFC);
            status = STATUS_SUCCESS;
            retlen = sizeof(*req);
        }
    }

    WdfRequestCompleteWithInformation(Request, status, retlen);
}
