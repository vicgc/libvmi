/* The LibVMI Library is an introspection library that simplifies access to
 * memory in a target virtual machine or in a file containing a dump of
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Author: Tamas K Lengyel (tamas.lengyel@zentific.com)
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>

#include <libvmi/libvmi.h>
#include <libvmi/libvmi_extra.h>

// Config values
addr_t win_tasks   = 0x2e8;
addr_t win_pdbase  = 0x28;
addr_t win_pid     = 0x2e0;
addr_t win_pname   = 0x438;
addr_t win_kpcr    = 0x2f6000;

// KDBG
KDDEBUGGER_DATA64 kdbg = {
    .Header.signature.v = VMI_WINDOWS_8_SIGNATURE,
    .PsActiveProcessHead = 0x296c10, // +KernBase
    .PsLoadedModuleList = 0x2caa60, // +KernBase
};

int main (int argc, char **argv)
{
    vmi_instance_t vmi;
    unsigned char *memory = NULL;
    uint32_t offset;
    addr_t list_head = 0, current_list_entry = 0, next_list_entry = 0;
    addr_t current_process = 0;
    addr_t tmp_next = 0;
    char *procname = NULL;
    vmi_pid_t pid = 0;
    status_t status;

    /* this is the VM or file that we are looking at */
    if (argc != 2) {
        printf("Usage: %s <vmname>\n", argv[0]);
        return 1;
    } // if

    char *name = argv[1];
    reg_t kpcr;

    /* get the FS/GS_BASE */
    if (vmi_init(&vmi, VMI_AUTO | VMI_INIT_PARTIAL, name) ==
        VMI_FAILURE) {
        printf("Failed to init LibVMI library.\n");
        goto error_exit;
    }

    page_mode_t pm = vmi_get_page_mode(vmi);
    if(VMI_PM_IA32E == pm) {
        if (VMI_FAILURE == vmi_get_vcpureg(vmi, &kpcr, GS_BASE, 0)) {
            goto error_exit;
        }
    } else {
        if (VMI_FAILURE == vmi_get_vcpureg(vmi, &kpcr, FS_BASE, 0)) {
            goto error_exit;
        }
    }

    vmi_destroy(vmi);

    kdbg.KernBase = kpcr - win_kpcr;
    kdbg.PsActiveProcessHead += kdbg.KernBase;
    kdbg.PsLoadedModuleList += kdbg.KernBase;

    GHashTable *config = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(config, "ostype", "Windows");
    g_hash_table_insert(config, "name", name);
    g_hash_table_insert(config, "win_tasks", &win_tasks);
    g_hash_table_insert(config, "win_pdbase", &win_pdbase);
    g_hash_table_insert(config, "win_pid", &win_pid);
    g_hash_table_insert(config, "win_pname", &win_pname);
    g_hash_table_insert(config, "win_kpcr", &win_kpcr);
    g_hash_table_insert(config, "win_kdbg_instance", &kdbg);

    /* initialize the libvmi library */
    if (vmi_init_custom(&vmi, VMI_AUTO | VMI_INIT_COMPLETE | VMI_CONFIG_GHASHTABLE, config) == VMI_FAILURE) {
        printf("Failed to init LibVMI library.\n");
        g_hash_table_destroy(config);
        return 1;
    }
    g_hash_table_destroy(config);

    /* pause the vm for consistent memory access */
    if (vmi_pause_vm(vmi) != VMI_SUCCESS) {
        printf("Failed to pause VM\n");
        goto error_exit;
    } // if

    /* demonstrate name and id accessors */
    char *name2 = vmi_get_name(vmi);

    if (VMI_FILE != vmi_get_access_mode(vmi)) {
        unsigned long id = vmi_get_vmid(vmi);

        printf("Process listing for VM %s (id=%lu)\n", name2, id);
    }
    else {
        printf("Process listing for file %s\n", name2);
    }
    free(name2);

    /* get the head of the list */
    if (VMI_OS_LINUX == vmi_get_ostype(vmi)) {
        /* Begin at PID 0, the 'swapper' task. It's not typically shown by OS
         *  utilities, but it is indeed part of the task list and useful to
         *  display as such.
         */
        current_process = vmi_translate_ksym2v(vmi, "init_task");
    }
    else if (VMI_OS_WINDOWS == vmi_get_ostype(vmi)) {

        // find PEPROCESS PsInitialSystemProcess
        vmi_read_addr_ksym(vmi, "PsInitialSystemProcess", &current_process);

    }

    /* walk the task list */
    list_head = current_process + win_tasks;
    current_list_entry = list_head;

    status = vmi_read_addr_va(vmi, current_list_entry, 0, &next_list_entry);
    if (status == VMI_FAILURE) {
        printf("Failed to read next pointer at 0x%"PRIx64" before entering loop\n",
                current_list_entry);
        goto error_exit;
    }

    printf("Next list entry is at: %"PRIx64"\n", next_list_entry);

    do {
        /* Note: the task_struct that we are looking at has a lot of
         * information.  However, the process name and id are burried
         * nice and deep.  Instead of doing something sane like mapping
         * this data to a task_struct, I'm just jumping to the location
         * with the info that I want.  This helps to make the example
         * code cleaner, if not more fragile.  In a real app, you'd
         * want to do this a little more robust :-)  See
         * include/linux/sched.h for mode details */

        /* NOTE: _EPROCESS.UniqueProcessId is a really VOID*, but is never > 32 bits,
         * so this is safe enough for x64 Windows for example purposes */
        vmi_read_32_va(vmi, current_process + win_pid, 0, &pid);

        procname = vmi_read_str_va(vmi, current_process + win_pname, 0);

        if (!procname) {
            printf("Failed to find procname\n");
            goto error_exit;
        }

        /* print out the process name */
        printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
        if (procname) {
            free(procname);
            procname = NULL;
        }

        current_list_entry = next_list_entry;
        current_process = current_list_entry - win_tasks;

        /* follow the next pointer */

        status = vmi_read_addr_va(vmi, current_list_entry, 0, &next_list_entry);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", current_list_entry);
            goto error_exit;
        }

    } while (next_list_entry != list_head);

    error_exit: if (procname)
        free(procname);

    /* resume the vm */
    vmi_resume_vm(vmi);

    /* cleanup any memory associated with the LibVMI instance */
    vmi_destroy(vmi);

    return 0;
}
