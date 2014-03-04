/* The LibVMI Library is an introspection library that simplifies access to
 * memory in a target virtual machine or in a file containing a dump of
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
 * retains certain rights in this software.
 *
 * Author: Tamas K Lengyel (tamas.k.lengyel@tum.de)
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
#include "private.h"
#include "driver/interface.h"
#include "arch/arm.h"
#include <glib.h>
#include <stdlib.h>
#include <sys/mman.h>

static inline
uint32_t get_bits_31to10 (uint32_t value)
{
    return value & 0xFFFFFC00UL;
}

static inline
uint32_t get_bits_31to12 (uint32_t value)
{
    return value & 0xFFFFF000UL;
}

static inline
uint32_t get_bits_31to14 (uint32_t value)
{
    return value & 0xFFFFE000UL;
}

static inline
uint32_t get_bits_31to16 (uint32_t value)
{
    return value & 0xFFFF0000UL;
}

static inline
uint32_t get_bits_31to20 (uint32_t value)
{
    return value & 0xFFF00000UL;
}

static inline
uint32_t get_bits_31to24 (uint32_t value)
{
    return value & 0xFF000000UL;
}

static inline
uint32_t get_bits_7to0 (uint32_t value)
{
    return value & 0x000000FFUL;
}

static inline
uint32_t get_bits_9to0 (uint32_t value)
{
    return value & 0x000003FFUL;
}

static inline
uint32_t get_bits_11to0 (uint32_t value)
{
    return value & 0x00000FFFUL;
}

static inline
uint32_t get_bits_15to0 (uint32_t value)
{
    return value & 0x0000FFFFUL;
}

static inline
uint32_t get_bits_19to0 (uint32_t value)
{
    return value & 0x000FFFFFUL;
}

static inline
uint32_t get_bits_23to0 (uint32_t value)
{
    return value & 0x00FFFFFFUL;
}

static inline
uint32_t first_level_table_index(uint32_t vaddr) {
    return (vaddr >> 20);
}

// 1st Level Descriptor
static inline
void get_first_level_descriptor(vmi_instance_t vmi, uint32_t dtb, uint32_t vaddr, page_info_t *info) {
    info->l1_a = get_bits_31to14(dtb) | (first_level_table_index(vaddr) << 2);
    uint32_t fld_v;
    if(VMI_SUCCESS == vmi_read_32_pa(vmi, info->l1_a, &fld_v)) {
        info->l1_v = fld_v;
    }
}

// 2nd Level Page Table Index (Course Pages)
static inline
uint32_t coarse_second_level_table_index(uint32_t vaddr) {
    return get_bits_7to0(vaddr >> 12);
}

// 2nd Level Page Table Descriptor (Course Pages)
static inline
void get_coarse_second_level_descriptor(vmi_instance_t vmi, uint32_t vaddr, page_info_t *info) {
    info->l2_a = get_bits_31to10(info->l1_v) | (coarse_second_level_table_index(vaddr) << 2);
    uint32_t sld_v;
    if(VMI_SUCCESS == vmi_read_32_pa(vmi, info->l2_a, &sld_v)) {
        info->l2_v = sld_v;
    }
}

// 2nd Level Page Table Index (Fine Pages)
static inline
uint32_t fine_second_level_table_index(uint32_t vaddr) {
    return get_bits_9to0(vaddr >> 10);
}

// 2nd Level Page Table Descriptor (Fine Pages)
static inline
void get_fine_second_level_descriptor(vmi_instance_t vmi, uint32_t vaddr, page_info_t *info) {
    info->l2_a = get_bits_31to12(info->l1_v) | fine_second_level_table_index(vaddr) | 0b11;
    uint32_t sld_v;
    if(VMI_SUCCESS == vmi_read_32_pa(vmi, info->l2_a, &sld_v)) {
        info->l2_v = sld_v;
    }
}

// Based on ARM Reference Manual
// Chapter B4 Virtual Memory System Architecture
// B4.7 Hardware page table translation
addr_t v2p_arm (vmi_instance_t vmi,
    addr_t dtb,
    addr_t vaddr,
    page_info_t *info)
{

    dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: vaddr = 0x%.16"PRIx64", dtb = 0x%.16"PRIx64"\n", vaddr, dtb);

    get_first_level_descriptor(vmi, dtb, vaddr, info);

    dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: l1d = 0x%"PRIx32"\n", info->l1_v);

    switch(info->l1_v & 0b11) {

        case 0b01: {

            dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: the entry gives the physical address of a coarse second-level table\n");

            get_coarse_second_level_descriptor(vmi, vaddr, info);

            dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: l2d = 0x%"PRIx32"\n", info->l2_v);

            switch(info->l2_v & 0b11) {
                case 0b01:
                    // large page
                    info->size = VMI_PS_64KB;
                    info->paddr = get_bits_31to16(info->l2_v) | get_bits_15to0(vaddr);
                    break;
                case 0b10:
                case 0b11:
                    // small page
                    info->size = VMI_PS_4KB;
                    info->paddr = get_bits_31to12(info->l2_v) | get_bits_11to0(vaddr);
                default:
                    break;
            }
        }

        case 0b10: {

            switch(VMI_GET_BIT(info->l1_v, 18)) {
            default:
            case 0:
                dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: the entry is a section descriptor for its associated modified virtual addresses\n");
                info->size = VMI_PS_1MB;
                info->paddr = get_bits_31to20(info->l1_v) | get_bits_19to0(vaddr);
                break;
            case 1:
                dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: the entry is a supersection descriptor for its associated modified virtual addresses\n");
                info->size = VMI_PS_16MB;
                //info->paddr = get_bits_31to24(info->l1_v) | get_bits_23to0(vaddr);
                break;
            }

            break;
        }

        case 0b11: {

            dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: the entry gives the physical address of a fine second-level table\n");

            get_fine_second_level_descriptor(vmi, vaddr, info);

            dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: sld = 0x%"PRIx32"\n", info->l2_v);

            switch(info->l1_v & 0b11) {
                case 0b01:
                    // large page
                    info->size = VMI_PS_64KB;
                    info->paddr = get_bits_31to16(info->l2_v) | get_bits_15to0(vaddr);
                    break;
                case 0b10:
                    // small page
                    info->size = VMI_PS_4KB;
                    info->paddr = get_bits_31to12(info->l2_v) | get_bits_11to0(vaddr);
                    break;
                case 0b11:
                    // tiny page
                    info->size = VMI_PS_1KB;
                    info->paddr = get_bits_31to10(info->l2_v) | get_bits_9to0(vaddr);
                    break;
                default:
                    break;
            }
        }

        default:
            break;
    }

    dbprint(VMI_DEBUG_PTLOOKUP, "--ARM PTLookup: PA = 0x%"PRIx64"\n", info->paddr);
    return info->paddr;
}

GSList* get_va_pages_arm(vmi_instance_t vmi, addr_t dtb) {
    return NULL;
}

status_t arm_init(vmi_instance_t vmi) {

    if(!vmi->arch_interface) {
        vmi->arch_interface = safe_malloc(sizeof(struct arch_interface));
        bzero(vmi->arch_interface, sizeof(struct arch_interface));
    }

    vmi->arch_interface->v2p = v2p_arm;
    vmi->arch_interface->get_va_pages = get_va_pages_arm;

    return VMI_SUCCESS;
}
