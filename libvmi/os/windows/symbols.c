/* The LibVMI Library is an introspection library that simplifies access to
 * memory in a target virtual machine or in a file containing a dump of
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
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

#include "private.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "os/linux/linux.h"

#define MAX_ROW_LENGTH 500

static status_t
get_symbol_row(
    FILE * f,
    char **row,
    const char *symbol,
    int position)
{

    status_t ret = VMI_FAILURE;
    size_t length = snprintf(NULL, 0, "\"%s\":", symbol);

    if(length > MAX_ROW_LENGTH) {
        errprint("Symbol length is too long!\n");
        return ret;
    }

    char *search = alloca(length + 1);
    sprintf(search, "\"%s\":", symbol);
    search[length] = '\0';

    while (fgets((*row), MAX_ROW_LENGTH, f) != NULL) {
        char *token = NULL;

        /* find the correct token to check */
        int curpos = 0;
        int position_copy = position;

        while (position_copy > 0 && curpos < MAX_ROW_LENGTH) {
            if (isspace((*row)[curpos])) {
                while (isspace((*row)[curpos])) {
                    (*row)[curpos] = '\0';
                    ++curpos;
                }
                --position_copy;
                continue;
            }
            ++curpos;
        }
        if (position_copy == 0) {
            token = (*row) + curpos;
            while (curpos < MAX_ROW_LENGTH) {
                if (isspace((*row)[curpos])) {
                    (*row)[curpos] = '\0';
                }
                ++curpos;
            }
        }
        else {  /* some went wrong in the loop above */
            goto error_exit;
        }

        /* check the token */
        if (strncmp(token, search, length) == 0) {
            (*row) = token + length + 1;

            // skip ahead if current row starts with '['
            if((*row)[0] == 91) {
                (*row)++;
            }
            ret = VMI_SUCCESS;
            break;
        }
    }

error_exit:
    if (ret == VMI_FAILURE) {
        memset(row, 0, MAX_ROW_LENGTH);
    }
    return ret;
}

status_t
windows_system_map_symbol_to_address(
    vmi_instance_t vmi,
    const char *symbol,
    const char *subsymbol,
    const addr_t kernel_base_vaddr,
    addr_t *address)
{
    FILE *f = NULL;
    char *row = NULL;
    status_t ret;

    windows_instance_t windows_instance = vmi->os_data;

    if (windows_instance == NULL) {
        errprint("VMI_ERROR: OS instance not initialized\n");
        return 0;
    }

    if ((NULL == windows_instance->sysmap) || (strlen(windows_instance->sysmap) == 0)) {
        errprint("VMI_WARNING: No linux sysmap configured\n");
        return 0;
    }

    row = safe_malloc(MAX_ROW_LENGTH);
    if ((f = fopen(windows_instance->sysmap, "r")) == NULL) {
        fprintf(stderr,
                "ERROR: could not find Windows system map file after checking:\n");
        fprintf(stderr, "\t%s\n", windows_instance->sysmap);
        fprintf(stderr,
                "To fix this problem, add the correct sysmap entry to /etc/libvmi.conf\n");
        address = 0;
        goto error_exit;
    }

    if (get_symbol_row(f, &row, symbol, 1) == VMI_FAILURE) {
        address = 0;
        goto error_exit;
    }

    if(subsymbol) {
        if (get_symbol_row(f, &row, subsymbol, 1) == VMI_FAILURE) {
            address = 0;
            goto error_exit;
        }
    }

    (*address) = (addr_t) strtoull(row, NULL, 10);

    return VMI_SUCCESS;
error_exit:
    if (row)
        free(row);
    if (f)
        fclose(f);
    return VMI_FAILURE;
}
