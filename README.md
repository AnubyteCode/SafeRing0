____

**SafeRing0: A secure/limited alternative to WinRing0**
=
____
***The CVE-2020-14979 vulnerability arose because WinRing0 exposed a device object with a NULL DACL and 
undocumented IOCTLs that let any user open \Device\PhysicalMemory and read/write arbitrary addresses.***
____
 
**For contrast, SafeRing0 is limited to the following as of now:**

*Only creates the KMDF device \Device\SafeRing0 with the framework’s default security descriptor (not a NULL DACL).*

*Exposes exactly five well-defined IOCTLs—MSR read, PCI read/write (strictly limited to bus 0, dev 0, func 0, offset 0x60), PMC read, and affinity-pinned MSR read.*

*Never calls MmMapIoSpace, ZwOpenSection, or any mapping function on \Device\PhysicalMemory.*

*Validates all PCI writes against a whitelist (bus 0, device 0, function 0, offset 0x60), returning STATUS_ACCESS_DENIED for anything else.*

____

Because neither a raw physical-memory IOCTL is exposed nor is there a loosening of the ACL on your device object, 
you’re not vulnerable to the “map any physical page” escalation CVE-2020-14979 describes.

____

If you wish to add this to your project, please give credit somewhere, or ask me to make a pull req.
