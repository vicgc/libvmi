/* The LibVMI Library is an introspection library that simplifies access to
 * memory in a target virtual machine or in a file containing a dump of
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
 * retains certain rights in this software.
 *
 * Author: Bryan D. Payne (bdpayne@acm.org)
 * Author: Tamas K Lengyel(tamas.lengyel@zentific.com)
 *
 * This file is part of LibVMI.
 *
 * LibVMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * LibVMI is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LibVMI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libvmi.h"
#include "peparse.h"
#include "private.h"
#define _GNU_SOURCE
#include <string.h>

static status_t
kdbg_symbol_resolve(
    vmi_instance_t vmi,
    unsigned long offset,
    addr_t *address)
{
    uint64_t tmp = 0;
    addr_t symaddr = 0;
    windows_instance_t windows = NULL;

    if (vmi->os_data == NULL) {
        return VMI_FAILURE;
    }

    windows = vmi->os_data;

    if(windows->kdbg) {
        *address = *((unsigned long *)windows->kdbg + offset);
        return VMI_SUCCESS;
    }

    symaddr = windows->kdbg_va + offset;

    if (VMI_FAILURE == vmi_read_64_va(vmi, symaddr, 0, &tmp)) {
        return VMI_FAILURE;
    }
    *address = tmp;
    return VMI_SUCCESS;
}

static status_t
kdbg_symbol_offset(
    vmi_instance_t vmi,
    const char *symbol,
    unsigned long *offset)
{
    KDDEBUGGER_DATA64 d;
    unsigned long max_symbol_length = 50;

    if (strncmp(symbol, "KernBase", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KernBase)) - (unsigned long) (&d);
    }
    else if (strncmp(symbol, "BreakpointWithStatus", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.BreakpointWithStatus)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "SavedContext", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.SavedContext)) - (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KiCallUserMode", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KiCallUserMode)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "KeUserCallbackDispatcher",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KeUserCallbackDispatcher)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "PsLoadedModuleList", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.PsLoadedModuleList)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "PsActiveProcessHead", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.PsActiveProcessHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "PspCidTable", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.PspCidTable)) - (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "ExpSystemResourcesList",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.ExpSystemResourcesList)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "ExpPagedPoolDescriptor",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.ExpPagedPoolDescriptor)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "ExpNumberOfPagedPools", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.ExpNumberOfPagedPools)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KeTimeIncrement", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KeTimeIncrement)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "KeBugCheckCallbackListHead",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KeBugCheckCallbackListHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KiBugcheckData", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KiBugcheckData)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "IopErrorLogListHead", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.IopErrorLogListHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "ObpRootDirectoryObject",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.ObpRootDirectoryObject)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "ObpTypeObjectType", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.ObpTypeObjectType)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSystemCacheStart", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmSystemCacheStart)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSystemCacheEnd", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmSystemCacheEnd)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSystemCacheWs", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmSystemCacheWs)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmPfnDatabase", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmPfnDatabase)) - (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSystemPtesStart", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmSystemPtesStart)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSystemPtesEnd", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmSystemPtesEnd)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSubsectionBase", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmSubsectionBase)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmNumberOfPagingFiles", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmNumberOfPagingFiles)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmLowestPhysicalPage", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmLowestPhysicalPage)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmHighestPhysicalPage", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmHighestPhysicalPage)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmNumberOfPhysicalPages",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmNumberOfPhysicalPages)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmMaximumNonPagedPoolInBytes",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmMaximumNonPagedPoolInBytes)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmNonPagedSystemStart", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmNonPagedSystemStart)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmNonPagedPoolStart", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmNonPagedPoolStart)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmNonPagedPoolEnd", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmNonPagedPoolEnd)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmPagedPoolStart", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmPagedPoolStart)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmPagedPoolEnd", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmPagedPoolEnd)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmPagedPoolInformation",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmPagedPoolInformation)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmPageSize", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmPageSize)) - (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmSizeOfPagedPoolInBytes",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmSizeOfPagedPoolInBytes)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmTotalCommitLimit", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmTotalCommitLimit)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmTotalCommittedPages", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmTotalCommittedPages)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSharedCommit", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmSharedCommit)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmDriverCommit", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmDriverCommit)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmProcessCommit", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmProcessCommit)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmPagedPoolCommit", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmPagedPoolCommit)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmExtendedCommit", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmExtendedCommit)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmZeroedPageListHead", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmZeroedPageListHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmFreePageListHead", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmFreePageListHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmStandbyPageListHead", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmStandbyPageListHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmModifiedPageListHead",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmModifiedPageListHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmModifiedNoWritePageListHead",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmModifiedNoWritePageListHead)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmAvailablePages", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmAvailablePages)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmResidentAvailablePages",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmResidentAvailablePages)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "PoolTrackTable", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.PoolTrackTable)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "NonPagedPoolDescriptor",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.NonPagedPoolDescriptor)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmHighestUserAddress", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmHighestUserAddress)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSystemRangeStart", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmSystemRangeStart)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmUserProbeAddress", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmUserProbeAddress)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KdPrintCircularBuffer", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.KdPrintCircularBuffer)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "KdPrintCircularBufferEnd",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KdPrintCircularBufferEnd)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KdPrintWritePointer", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.KdPrintWritePointer)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KdPrintRolloverCount", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.KdPrintRolloverCount)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmLoadedUserImageList", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmLoadedUserImageList)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "NtBuildLab", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.NtBuildLab)) - (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KiNormalSystemCall", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.KiNormalSystemCall)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KiProcessorBlock", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.KiProcessorBlock)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmUnloadedDrivers", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmUnloadedDrivers)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmLastUnloadedDriver", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmLastUnloadedDriver)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmTriageActionTaken", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmTriageActionTaken)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSpecialPoolTag", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmSpecialPoolTag)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KernelVerifier", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KernelVerifier)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmVerifierData", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmVerifierData)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmAllocatedNonPagedPool",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmAllocatedNonPagedPool)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmPeakCommitment", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.MmPeakCommitment)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmTotalCommitLimitMaximum",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmTotalCommitLimitMaximum)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "CmNtCSDVersion", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.CmNtCSDVersion)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmPhysicalMemoryBlock", max_symbol_length)
             == 0) {
        *offset =
            (unsigned long) (&(d.MmPhysicalMemoryBlock)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSessionBase", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmSessionBase)) - (unsigned long) (&d);
    }
    else if (strncmp(symbol, "MmSessionSize", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmSessionSize)) - (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmSystemParentTablePage",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmSystemParentTablePage)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "MmVirtualTranslationBase",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.MmVirtualTranslationBase)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "KdPrintCircularBufferPtr",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KdPrintCircularBufferPtr)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KdPrintBufferSize", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.KdPrintBufferSize)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "KeLoaderBlock", max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.KeLoaderBlock)) - (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "IopNumTriageDumpDataBlocks",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.IopNumTriageDumpDataBlocks)) -
            (unsigned long) (&d);
    }
    else if (strncmp
             (symbol, "IopTriageDumpDataBlocks",
              max_symbol_length) == 0) {
        *offset =
            (unsigned long) (&(d.IopTriageDumpDataBlocks)) -
            (unsigned long) (&d);
    }
    else if (strncmp(symbol, "VfCrashDataBlock", max_symbol_length) ==
             0) {
        *offset =
            (unsigned long) (&(d.VfCrashDataBlock)) -
            (unsigned long) (&d);
    }
    else {
        return VMI_FAILURE;
    }

    return VMI_SUCCESS;
}

// Idea from http://gleeda.blogspot.com/2010/12/identifying-memory-images.html
win_ver_t
find_windows_version(
    vmi_instance_t vmi,
    addr_t kdbg)
{

    dbprint(VMI_DEBUG_MISC, "--Find Windows version from KDBG @ 0x%lx\n", kdbg);

    windows_instance_t windows = NULL;

    if (vmi->os_data == NULL) {
        return VMI_FAILURE;
    }

    windows = vmi->os_data;

    // no need to repeat this work if we already have the answer
    if (windows->version &&
        windows->version != VMI_OS_WINDOWS_UNKNOWN) {
        return windows->version;
    }

    win_ver_t version = VMI_OS_WINDOWS_UNKNOWN;
    vmi_read_16_pa(vmi, kdbg + 0x14, (uint16_t *)&version);

    // Check if it's a version we know about.
    // The known KDBG magic values are defined in win_ver_t
    switch(version) {
        case VMI_OS_WINDOWS_2000:
        case VMI_OS_WINDOWS_XP:
        case VMI_OS_WINDOWS_2003:
        case VMI_OS_WINDOWS_VISTA:
        case VMI_OS_WINDOWS_2008:
        case VMI_OS_WINDOWS_7:
        case VMI_OS_WINDOWS_8:
            break;
        default:
            version = VMI_OS_WINDOWS_UNKNOWN;
            break;
    }

    return version;
}

status_t find_kdbg_address(
    vmi_instance_t vmi,
    addr_t *kdbg_pa,
    addr_t *kernel_va)
{

    dbprint(VMI_DEBUG_MISC, "**Trying find_kdbg_address\n");

    status_t ret = VMI_FAILURE;
    *kdbg_pa = 0;
    addr_t paddr = 0;
    unsigned char haystack[VMI_PS_4KB];
    addr_t memsize = vmi_get_memsize(vmi);

    void *bm64 = boyer_moore_init("\x00\xf8\xff\xffKDBG", 8);
    void *bm32 = boyer_moore_init("\x00\x00\x00\x00\x00\x00\x00\x00KDBG", 12);
    uint32_t find_ofs_64 = 0xc, find_ofs_32 = 0x8, find_ofs = 0;

    for(;paddr<memsize;paddr+=VMI_PS_4KB) {

        find_ofs = 0;

        if(VMI_PS_4KB != vmi_read_pa(vmi, paddr, &haystack, VMI_PS_4KB)) {
            continue;
        }

        int match_offset = boyer_moore2(bm64, haystack, VMI_PS_4KB);
        if (-1 != match_offset) {
            find_ofs = find_ofs_64;
        } else {
            match_offset = boyer_moore2(bm32, haystack, VMI_PS_4KB);
        }

        if (-1 != match_offset) {

            if(!find_ofs) {
                find_ofs = find_ofs_32;
            }

            // Read "KernBase" from the haystack
            long unsigned int kernbase_offset = 0;
            kdbg_symbol_offset(vmi, "KernBase", &kernbase_offset);
            *kernel_va = *(uint64_t *)&haystack[(unsigned int) match_offset - find_ofs + kernbase_offset];
            *kdbg_pa = paddr + (unsigned int) match_offset - find_ofs;

            ret = VMI_SUCCESS;

            break;
        }
    }

    dbprint(VMI_DEBUG_MISC, "--Found KdDebuggerDataBlock at PA %.16"PRIx64"\n", *kdbg_pa);

    ret = VMI_SUCCESS;

exit:
    return ret;
}

status_t
find_kdbg_address_fast(
    vmi_instance_t vmi,
    addr_t *kdbg_pa,
    addr_t *kernel_pa,
    addr_t *kernel_va)
{

    dbprint(VMI_DEBUG_MISC, "**Trying find_kdbg_address_fast\n");

    status_t ret = VMI_FAILURE;
    reg_t cr3;
    if(VMI_FAILURE == driver_get_vcpureg(vmi, &cr3, CR3, 0)) {
        return ret;
    }

    addr_t memsize = vmi_get_memsize(vmi);
    GSList *va_pages = vmi_get_va_pages(vmi, (addr_t)cr3);
    size_t read = 0;
    void *bm = 0;   // boyer-moore internal state
    unsigned char haystack[VMI_PS_4KB];
    int find_ofs = 0;

    if (VMI_PM_IA32E == vmi->page_mode) {
        bm = boyer_moore_init("\x00\xf8\xff\xffKDBG", 8);
        find_ofs = 0xc;
    }
    else {
        bm = boyer_moore_init("\x00\x00\x00\x00\x00\x00\x00\x00KDBG",
                              12);
        find_ofs = 0x8;
    }   // if-else

    GSList *va_pages_loop = va_pages;
    while(va_pages_loop) {

        struct va_page *vap = (struct va_page *)va_pages_loop->data;

        // We might get pages that are greater than 4Kb
        // so we are just going to split them to 4Kb pages
        while(vap && vap->size >= VMI_PS_4KB) {
            vap->size -= VMI_PS_4KB;
            addr_t page_vaddr = vap->va+vap->size;
            addr_t page_paddr = vmi_pagetable_lookup(vmi, cr3, page_vaddr);

            if(page_paddr + VMI_PS_4KB - 1 > memsize) {
                continue;
            }

            read = vmi_read_pa(vmi, page_paddr, haystack, VMI_PS_4KB);

            if (VMI_PS_4KB != read) {
                continue;
            }

            int match_offset = boyer_moore2(bm, haystack, VMI_PS_4KB);

            if (-1 != match_offset) {

                addr_t k_va = 0;
                if(VMI_FAILURE == vmi_read_addr_pa(vmi, page_paddr + (unsigned int) match_offset + sizeof(uint64_t), &k_va)) {
                    continue;
                }

                *kernel_va = k_va;
                *kernel_pa = vmi_pagetable_lookup(vmi, cr3, *kernel_va);
                *kdbg_pa = page_paddr + (unsigned int) match_offset - find_ofs;

                ret = VMI_SUCCESS;

                goto done;
            }
        }
        g_free(vap);
        va_pages_loop = va_pages_loop->next;
    }

done:
    // free the rest of the list
    while(va_pages_loop) {
        g_free(va_pages_loop->data);
        va_pages_loop = va_pages_loop->next;
    }
    g_slist_free(va_pages);

    if (VMI_SUCCESS == ret)
        dbprint(VMI_DEBUG_MISC, "--Found KdDebuggerDataBlock at PA %.16"PRIx64"\n", *kdbg_pa);
    boyer_moore_fini(bm);
    return ret;
}

status_t
find_kdbg_address_faster(
    vmi_instance_t vmi,
    addr_t *kdbg_pa,
    addr_t *kernel_pa,
    addr_t *kernel_va)
{

    dbprint(VMI_DEBUG_MISC, "**Trying find_kdbg_address_faster\n");

    status_t ret = VMI_FAILURE;

    // This scan requires the location of the KPCR
    // which we get from the GS/FS register on live machines.
    // For file mode this needs to be further investigated.
    if(VMI_FILE == vmi->mode) {
        return ret;
    }

    void *bm = boyer_moore_init("KDBG", 4);
    int find_ofs = 0x10;

    reg_t cr3, fsgs;
    if(VMI_FAILURE == driver_get_vcpureg(vmi, &cr3, CR3, 0)) {
        return ret;
    }

    if (VMI_PM_IA32E == vmi->page_mode) {
        if(VMI_FAILURE == driver_get_vcpureg(vmi, &fsgs, GS_BASE, 0)) {
            return ret;
        }
    } else if(VMI_PM_LEGACY == vmi->page_mode || VMI_PM_PAE == vmi->page_mode) {
        if(VMI_FAILURE == driver_get_vcpureg(vmi, &fsgs, FS_BASE, 0)) {
            return ret;
        }
    }

    // We start the search from the KPCR, which has to be mapped into the kernel.
    // We further know that the Windows kernel is page aligned
    // so we are just checking if the page has a valid PE header
    // and if the first item in the export table is "ntoskrnl.exe".
    // Once the kernel is found, we find the .data section
    // and limit the string search for "KDBG" into that region.

    // start searching at the lower part from the kpcr
    // then switch to the upper part if needed
    int step = -VMI_PS_4KB;
    addr_t page_paddr = 0;

scan:
    page_paddr = (vmi_pagetable_lookup(vmi, cr3, fsgs) >> 12) << 12;
    for(; page_paddr + step >= 0 && page_paddr + step < vmi->size ; page_paddr += step) {

        uint8_t page[VMI_PS_4KB];
        status_t rc = peparse_get_image_phys(vmi, page_paddr, VMI_PS_4KB, page);
        if(VMI_FAILURE == rc) {
            continue;
        }

        struct pe_header *pe_header = NULL;
        struct dos_header *dos_header = NULL;
        void *optional_pe_header = NULL;
        uint16_t optional_header_type = 0;
        struct export_table et;

        peparse_assign_headers(page, &dos_header, &pe_header, &optional_header_type, &optional_pe_header, NULL, NULL);
        addr_t export_header_offset =
            peparse_get_idd_rva(IMAGE_DIRECTORY_ENTRY_EXPORT, &optional_header_type, optional_pe_header, NULL, NULL);

        if(!export_header_offset || page_paddr + export_header_offset > vmi->size)
            continue;

        uint32_t nbytes = vmi_read_pa(vmi, page_paddr + export_header_offset, &et, sizeof(struct export_table));
        if(nbytes == sizeof(struct export_table) && !(et.export_flags || !et.name) ) {

            if(page_paddr + et.name + 12 > vmi->size)
                continue;

            unsigned char name[13] = {0};
            vmi_read_pa(vmi, page_paddr + et.name, name, 12);
            if(strcmp("ntoskrnl.exe", name)) {
                continue;
            }
        } else {
            continue;
        }

        uint32_t c;
        for(c=0; c < pe_header->number_of_sections; c++) {

            struct section_header section;
            addr_t section_addr = page_paddr
                + dos_header->offset_to_pe
                + sizeof(struct pe_header)
                + pe_header->size_of_optional_header
                + c*sizeof(struct section_header);

            // Read the section header from memory
            vmi_read_pa(vmi, section_addr, (uint8_t *)&section, sizeof(struct section_header));

            // .data check
            if(memcmp(section.short_name, "\x2E\x64\x61\x74\x61", 5) != 0) {
                continue;
            }

            uint8_t *haystack = alloca(section.size_of_raw_data);
            vmi_read_pa(vmi, page_paddr + section.virtual_address, haystack, section.size_of_raw_data);

            int match_offset = boyer_moore2(bm, haystack, section.size_of_raw_data);

            if (-1 != match_offset) {
                // We found the structure, but let's verify it.
                // The kernel is always mapped into VA at the same offset
                // it is found on physical memory + the kernel boundary.

                // Read "KernBase" from the haystack
                uint64_t *kernbase = (uint64_t *)&haystack[(unsigned int) match_offset + sizeof(uint64_t)];
                int zeroes = __builtin_clzll(page_paddr);

                if((*kernbase) << zeroes == page_paddr << zeroes) {

                    *kernel_pa = page_paddr;
                    *kernel_va = *kernbase;
                    *kdbg_pa = page_paddr + section.virtual_address + (unsigned int) match_offset - find_ofs;

                    ret = VMI_SUCCESS;

                    dbprint(VMI_DEBUG_MISC,
                        "--Found KdDebuggerDataBlock at PA %.16"PRIx64"\n", *kdbg_pa);

                    goto done;
                } else {
                    dbprint(VMI_DEBUG_MISC,
                        "--WARNING: KernBase in KdDebuggerDataBlock at PA %.16"PRIx64" doesn't point back to this page.\n",
                        page_paddr + section.virtual_address + (unsigned int) match_offset - find_ofs);
                }
            }

            break;
        }
    }

    if(step<0) {
        step = VMI_PS_4KB;
        goto scan;
    }

done:
    boyer_moore_fini(bm);
    return ret;
}

status_t
find_kdbg_address_instant(
    vmi_instance_t vmi,
    addr_t *kdbg_pa,
    addr_t *kernel_pa,
    addr_t *kernel_va)
{

    dbprint(VMI_DEBUG_MISC, "**Trying find_kdbg_address_instant\n");

    status_t ret = VMI_FAILURE;
    windows_instance_t windows = NULL;
    if (vmi->os_data == NULL) {
        goto done;
    }

    windows = vmi->os_data;

    // If the kernel base is unknown this approach requires the
    // location of the KPCR which we get from the GS/FS register,
    // available only on live machines.
    if (VMI_FILE == vmi->mode) {
        goto done;
    }

    // We also need the config settings for the RVAs
    if (!windows->kdbg_offset || !windows->kpcr_offset) {
        goto done;
    }

    reg_t cr3, fsgs;
    if (VMI_FAILURE == driver_get_vcpureg(vmi, &cr3, CR3, 0)) {
        goto done;
    }

    if (VMI_PM_IA32E == vmi->page_mode) {
        driver_get_vcpureg(vmi, &fsgs, GS_BASE, 0);
    } else {
        driver_get_vcpureg(vmi, &fsgs, FS_BASE, 0);
    }

    addr_t kernelbase_va = fsgs - windows->kpcr_offset;
    addr_t kernelbase_pa = vmi_pagetable_lookup(vmi, cr3, kernelbase_va);

    if (!kernelbase_pa) {
        goto done;
    }

    *kernel_pa = kernelbase_pa;
    *kernel_va = kernelbase_va;
    *kdbg_pa = kernelbase_pa + windows->kdbg_offset;

    ret = VMI_SUCCESS;

done:
    return ret;

}

status_t
windows_kdbg_lookup(
    vmi_instance_t vmi,
    const char *symbol,
    addr_t *address)
{
    status_t ret = VMI_FAILURE;
    unsigned long offset = 0;

    if (VMI_FAILURE == kdbg_symbol_offset(vmi, symbol, &offset)) {
        goto done;
    }
    if (VMI_FAILURE == kdbg_symbol_resolve(vmi, offset, address)) {
        goto done;
    }

    ret = VMI_SUCCESS;

done:
    return ret;
}

/*
 * This functions is responsible for setting up
 * Windows specific variables:
 *  - ntoskrnl (*)
 *  - ntoskrnl_va
 *  - kdbg_offset (*)
 *  - kdbg_va (*)
 * The variables marked with (*) can be also specified
 * in the libvmi config.
 */
status_t
init_kdbg(
    vmi_instance_t vmi)
{
    status_t ret = VMI_FAILURE;

    if (vmi->os_data == NULL) {
        goto exit;
    }

    windows_instance_t windows = vmi->os_data;

    // Lets try to init from the config settings passed to us by the user
    if(windows->kdbg_va) {
        if(VMI_SUCCESS == windows_kdbg_lookup(vmi, "KernBase", &windows->ntoskrnl_va)) {

            dbprint(VMI_DEBUG_MISC, "**KernBase VA=0x%"PRIx64"\n", windows->ntoskrnl_va);

            if(!windows->ntoskrnl) {
                windows->ntoskrnl = vmi_translate_kv2p(vmi, windows->ntoskrnl_va);
                dbprint(VMI_DEBUG_MISC, "**KernBase PA=0x%"PRIx64"\n", windows->kdbg_offset);
            }

            if(!windows->kdbg_offset) {
                windows->kdbg_offset = windows->kdbg_va - windows->ntoskrnl_va;
                dbprint(VMI_DEBUG_MISC, "**kdbg_offset=0x%"PRIx64"\n", windows->kdbg_offset);
            }

            ret = VMI_SUCCESS;

            goto exit;
        } else {
            dbprint(VMI_DEBUG_MISC, "**Failed to get KernBase from KDBG set in config\n");
        }
    } else
    if(windows->ntoskrnl && windows->kdbg_offset) {
        if(!windows->ntoskrnl_va) {
            unsigned long offset;
            kdbg_symbol_offset(vmi, "KernBase", &offset);
            if(VMI_FAILURE == vmi_read_addr_pa(vmi, windows->ntoskrnl + windows->kdbg_offset + offset, &windows->ntoskrnl_va)) {
                errprint("Inconsistent addresses passed in the config!\n");
                goto exit;
            }

            if(vmi_translate_kv2p(vmi, windows->ntoskrnl_va) != windows->ntoskrnl) {
                errprint("Inconsistent addresses passed in the config!\n");
                goto exit;
            }
        }
        if(!windows->kdbg_va) {
            windows->kdbg_va = windows->ntoskrnl_va + windows->kdbg_offset;
        }
        ret = VMI_SUCCESS;
        goto exit;
    }

    // We don't have the standard config informations
    // so lets try our kdbg search methods
    addr_t kernbase_pa = 0, kernbase_va = 0, kdbg_pa = 0;

    if(VMI_SUCCESS == find_kdbg_address_instant(vmi, &kdbg_pa, &kernbase_pa, &kernbase_va)) {
        goto found;
    }
    if(VMI_SUCCESS == find_kdbg_address_faster(vmi, &kdbg_pa, &kernbase_pa, &kernbase_va)) {
        goto found;
    }
    if(VMI_SUCCESS == find_kdbg_address_fast(vmi, &kdbg_pa, &kernbase_pa, &kernbase_va)) {
        goto found;
    }
    if(VMI_SUCCESS == find_kdbg_address(vmi, &kdbg_pa, &kernbase_va)) {
        goto found;
    }

    dbprint(VMI_DEBUG_MISC, "**KdDebuggerDataBlock init failed\n");
    goto exit;

found:
    windows->version = find_windows_version(vmi, kdbg_pa);
    if(VMI_OS_WINDOWS_UNKNOWN == windows->version) {
        goto exit;
    }

    if(!kernbase_pa) {
        kernbase_pa = get_ntoskrnl_base(vmi, 0);
    }

    windows->ntoskrnl = kernbase_pa;
    windows->ntoskrnl_va = kernbase_va;
    windows->kdbg_offset = kdbg_pa - windows->ntoskrnl;
    windows->kdbg_va = windows->ntoskrnl_va + windows->kdbg_offset;

    printf("LibVMI Suggestion: set win_kdvb=0x%"PRIx64" in libvmi.conf for faster startup.\n",
             windows->ntoskrnl_va + windows->kdbg_offset);

    dbprint(VMI_DEBUG_MISC, "**set KdDebuggerDataBlock address=0x%"PRIx64"\n",
            windows->kdbg_va);

    ret = VMI_SUCCESS;

exit:
    return ret;
}
