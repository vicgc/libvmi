/* The LibVMI Library is an introspection library that simplifies access to
 * memory in a target virtual machine or in a file containing a dump of
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Copyright 2011 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
 * retains certain rights in this software.
 *
 * Author: Bryan D. Payne (bdpayne@acm.org)
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
#include "driver/memory_cache.h"
#include "os/os_interface.h"
#include "os/windows/windows.h"
#include "os/linux/linux.h"
#include "config/config_parser.h"
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <limits.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <pwd.h>

extern FILE *yyin;

static FILE *
open_config_file(
    )
{
    FILE *f = NULL;
    char location[100];
    char *sudo_user = NULL;
    struct passwd *pw_entry = NULL;

    /* first check home directory of sudo user */
    if ((sudo_user = getenv("SUDO_USER")) != NULL) {
        if ((pw_entry = getpwnam(sudo_user)) != NULL) {
            snprintf(location, 100, "%s/etc/libvmi.conf\0",
                     pw_entry->pw_dir);
            dbprint(VMI_DEBUG_CORE, "--looking for config file at %s\n", location);
            if ((f = fopen(location, "r")) != NULL) {
                goto success;
            }
        }
    }

    /* next check home directory for current user */
    snprintf(location, 100, "%s/etc/libvmi.conf\0", getenv("HOME"));
    dbprint(VMI_DEBUG_CORE, "--looking for config file at %s\n", location);
    if ((f = fopen(location, "r")) != NULL) {
        goto success;
    }

    /* finally check in /etc */
    snprintf(location, 100, "/etc/libvmi.conf\0");
    dbprint(VMI_DEBUG_CORE, "--looking for config file at %s\n", location);
    if ((f = fopen(location, "r")) != NULL) {
        goto success;
    }

    return NULL;
success:
    dbprint(VMI_DEBUG_CORE, "**Using config file at %s\n", location);
    return f;
}

status_t
set_os_type_from_config(
    vmi_instance_t vmi)
{
    status_t ret = VMI_FAILURE;
    GHashTable *configtbl = (GHashTable *)vmi->config;
    const char* ostype = NULL;

    vmi->os_type = VMI_OS_UNKNOWN;
    if (vmi->os_data) {
        free(vmi->os_data);
        vmi->os_data = NULL;
    }

    ostype = g_hash_table_lookup(configtbl, "ostype");
    if (ostype == NULL) {
        ostype = g_hash_table_lookup(configtbl, "os_type");
    }

    if (ostype == NULL) {
        errprint("Undefined OS type!\n");
        return VMI_FAILURE;
    }

    if (strncmp(ostype, "Linux", CONFIG_STR_LENGTH) == 0) {
        vmi->os_type = VMI_OS_LINUX;
        ret = VMI_SUCCESS;
    } else if (strncmp(ostype, "Windows", CONFIG_STR_LENGTH) == 0) {
        vmi->os_type = VMI_OS_WINDOWS;
        ret = VMI_SUCCESS;
    } else {
        errprint("VMI_ERROR: Unknown OS type: %s!\n", ostype);
        ret = VMI_FAILURE;
    }

    return ret;
}

status_t
read_config_file(
    vmi_instance_t vmi, FILE* config_file);

status_t read_config_string(vmi_instance_t vmi,
        const char *config) {
    status_t ret = VMI_SUCCESS;
    FILE* config_file = NULL;

    if (config == NULL) {
        errprint("VMI_ERROR: NULL string passed for VMI_CONFIG_STRING\n");
        return VMI_FAILURE;
    }

    int length = strlen(config) + strlen(vmi->image_type) + 2;
    char *config_str = safe_malloc(length);

    sprintf(config_str, "%s %s\0", vmi->image_type, config);

    config_file = fmemopen(config_str, strlen(config_str), "r");

    ret = read_config_file(vmi, config_file);

    return ret;
}

status_t read_config_file_entry(vmi_instance_t vmi) {
    status_t ret = VMI_FAILURE;
    FILE* config_file = NULL;

    config_file = open_config_file();
    if (NULL == config_file) {
        fprintf(stderr, "ERROR: config file not found.\n");
        ret = VMI_FAILURE;
        return ret;
    }

    ret = read_config_file(vmi, config_file);

    return ret;
}

status_t
read_config_file(
    vmi_instance_t vmi, FILE* config_file)
{
    status_t ret = VMI_SUCCESS;

    yyin = config_file;

    if (vmi_parse_config(vmi->image_type) != 0) {
        errprint("Failed to read config file.\n");
        ret = VMI_FAILURE;
        goto error_exit;
    }
    vmi->config = vmi_get_config();

    if (vmi->config == NULL) {
        errprint("No entry in config file for %s.\n", vmi->image_type);
        ret = VMI_FAILURE;
        goto error_exit;
    } else {
        ret = VMI_SUCCESS;
    }

#ifdef VMI_DEBUG
    if (vmi->os_type == VMI_OS_LINUX) {
        dbprint(VMI_DEBUG_CORE, "**set os_type to Linux.\n");
    }
    else if (vmi->os_type == VMI_OS_WINDOWS) {
        dbprint(VMI_DEBUG_CORE, "**set os_type to Windows.\n");
    }
    else {
        dbprint(VMI_DEBUG_CORE, "**set os_type to unknown.\n");
    }
#endif

error_exit:
    if (config_file)
        fclose(config_file);
    return ret;
}

static status_t
init_page_offset(
    vmi_instance_t vmi)
{
    //TODO need to actually determine these values instead of just guessing
    //TODO need a better way to handle the page size issue
    /* assume 4k pages for now, update when 2M page is found */
    vmi->page_shift = 12;
    vmi->page_size = 1 << vmi->page_shift;

    return VMI_SUCCESS;
}

static status_t
set_driver_type(
    vmi_instance_t vmi,
    vmi_mode_t mode,
    unsigned long id,
    char *name)
{
    if (VMI_AUTO == mode) {
        if (VMI_FAILURE == driver_init_mode(vmi, id, name)) {
            errprint("Failed to identify correct mode.\n");
            return VMI_FAILURE;
        }
    }
    else {
        vmi->mode = mode;
    }
    dbprint(VMI_DEBUG_CORE, "LibVMI Mode %d\n", vmi->mode);
    return VMI_SUCCESS;
}

/* the name passed may contain the full path and we just want the filename */
static void
set_image_type_for_file(
    vmi_instance_t vmi,
    char *name)
{
    char *ptr = NULL;

    if ((ptr = strrchr(name, '/')) == NULL) {
        ptr = name;
    }
    else {
        ptr++;
    }
    vmi->image_type = strndup(ptr, 500);
    vmi->image_type_complete = strndup(name, 500);
}

static status_t
set_id_and_name(
    vmi_instance_t vmi,
    vmi_mode_t mode,
    unsigned long id,
    char *name)
{
    if (VMI_FILE == vmi->mode) {
        if (name) {
            set_image_type_for_file(vmi, name);
            driver_set_name(vmi, name);
        }
        else {
            errprint("Must specify name for file mode.\n");
            return VMI_FAILURE;
        }
    }
    else {
        /* resolve and set id and name */
        if (VMI_INVALID_DOMID == id) {
            if (name) {
                if (VMI_INVALID_DOMID != (id = driver_get_id_from_name(vmi, name)) ) {
                    dbprint(VMI_DEBUG_CORE, "--got id from name (%s --> %lu)\n", name, id);
                    driver_set_id(vmi, id);
                } else {
                    errprint("Failed to get domain id from name.\n");
                    return VMI_FAILURE;
                }
            }
            else {
                errprint("Must specifiy either id or name.\n");
                return VMI_FAILURE;
            }
        }
        else {
            if (name) {
                errprint("Specifying both id and name is undefined.\n");
                return VMI_FAILURE;
            }

            if(VMI_FAILURE == driver_check_id(vmi,id)) {
                errprint("Invalid id.\n");
                return VMI_FAILURE;
            }

            driver_set_id(vmi, id);

            if (VMI_FAILURE != driver_get_name_from_id(vmi, id, &name)) {
                dbprint(VMI_DEBUG_CORE, "--got name from id (%lu --> %s)\n", id, name);
            } else {
                dbprint(VMI_DEBUG_CORE, "--failed to get domain name from id!\n");

                // Only under Xen this is OK
                if(vmi->mode != VMI_XEN) {
                    return VMI_FAILURE;
                }
            }
        }

       if(name != NULL) {
               vmi->image_type = strndup(name, 100);
               driver_set_name(vmi, name);
       } else {
               // create placeholder for image_type
               char *idstring = malloc(snprintf(NULL, 0, "domid-%lu", id) + 1);
               sprintf(idstring, "domid-%lu", id);
               vmi->image_type = idstring;
       }

    }
    dbprint(VMI_DEBUG_CORE, "**set image_type = %s\n", vmi->image_type);
    return VMI_SUCCESS;
}

static status_t
vmi_init_private(
    vmi_instance_t *vmi,
    uint32_t flags,
    unsigned long id,
    char *name,
    vmi_config_t *config)
{
    uint32_t access_mode = flags & 0x0000FFFF;
    uint32_t init_mode = flags & 0x00FF0000;
    uint32_t config_mode = flags & 0xFF000000;
    status_t status = VMI_FAILURE;

    /* allocate memory for instance structure */
    *vmi = (vmi_instance_t) safe_malloc(sizeof(struct vmi_instance));
    memset(*vmi, 0, sizeof(struct vmi_instance));

    /* initialize instance struct to default values */
    dbprint(VMI_DEBUG_CORE, "LibVMI Version 0.11.0\n");  //TODO change this with each release

    /* save the flags and init mode */
    (*vmi)->flags = flags;
    (*vmi)->init_mode = init_mode;
    (*vmi)->config_mode = config_mode;

    /* the config hash table is set up later based on mode */
    (*vmi)->config = NULL;

    /* set page mode to unknown */
    (*vmi)->page_mode = VMI_PM_UNKNOWN;

    /* setup the caches */
    pid_cache_init(*vmi);
    sym_cache_init(*vmi);
    rva_cache_init(*vmi);
    v2p_cache_init(*vmi);
#if ENABLE_SHM_SNAPSHOT == 1
    v2m_cache_init(*vmi);
#endif

    /* connecting to xen, kvm, file, etc */
    if (VMI_FAILURE == set_driver_type(*vmi, access_mode, id, name)) {
        goto error_exit;
    }

    /* resolve the id and name */
    if (VMI_FAILURE == set_id_and_name(*vmi, access_mode, id, name)) {
        goto error_exit;
    }

    /* driver-specific initilization */
    if (VMI_FAILURE == driver_init(*vmi)) {
        goto error_exit;
    }
    dbprint(VMI_DEBUG_CORE, "--completed driver init.\n");

    /* setup the page offset size */
    if (VMI_FAILURE == init_page_offset(*vmi)) {
        goto error_exit;
    }

    /* get the memory size */
    if (driver_get_memsize(*vmi, &(*vmi)->size) == VMI_FAILURE) {
        errprint("Failed to get memory size.\n");
        goto error_exit;
    }
    dbprint(VMI_DEBUG_CORE, "**set size = %"PRIu64" [0x%"PRIx64"]\n", (*vmi)->size,
        (*vmi)->size);

    // for file mode we need os-specific heuristics to deduce the architecture
    if (VMI_FILE != (*vmi)->mode) {
        /* architecture specific initilization */
        if(VMI_FAILURE == arch_init(*vmi)) {
           dbprint(VMI_DEBUG_CORE, "--failed to determine architecture.\n");
           goto error_exit;
        }

        dbprint(VMI_DEBUG_CORE, "--completed architecture init.\n");
    }

    /* we check VMI_INIT_COMPLETE first as
       VMI_INIT_PARTIAL is not exclusive */
    if (init_mode & VMI_INIT_COMPLETE) {

        switch((*vmi)->config_mode) {
            case VMI_CONFIG_STRING:
                /* read and parse the config string */
                if(VMI_FAILURE == read_config_string(*vmi, (char*)config)) {
                    goto error_exit;
                }
                break;
            case VMI_CONFIG_GLOBAL_FILE_ENTRY:
                /* read and parse the config file */
                if(VMI_FAILURE == read_config_file_entry(*vmi)) {
                    goto error_exit;
                }
                break;
            case VMI_CONFIG_GHASHTABLE:
                /* read and parse the ghashtable */
                if (!config) {
                    goto error_exit;
                }
                (*vmi)->config = (GHashTable*)config;
                break;
            case VMI_CONFIG_NONE:
            default:
                /* init_complete requires configuration
                   falling back to VMI_CONFIG_GLOBAL_FILE_ENTRY is unsafe here
                   as the config pointer is probably NULL */
                goto error_exit;
        }

        if(VMI_FAILURE == set_os_type_from_config(*vmi)) {
            dbprint(VMI_DEBUG_CORE, "--failed to determine os type from config\n");
            goto error_exit;
        }

        /* setup OS specific stuff */
        if (VMI_OS_LINUX == (*vmi)->os_type) {
            if(VMI_FAILURE == linux_init(*vmi)) {
                goto error_exit;
            }
        }
        else if (VMI_OS_WINDOWS == (*vmi)->os_type) {
            if(VMI_FAILURE == windows_init(*vmi)) {
                goto error_exit;
            }
        } else {
            goto error_exit;
        }

        status = VMI_SUCCESS;

    } else if (init_mode & VMI_INIT_PARTIAL) {

        status = VMI_SUCCESS;

    } else {

        errprint("Need to specify either VMI_INIT_PARTIAL or VMI_INIT_COMPLETE.\n");
        goto error_exit;

    }

    /* Enable event handlers */
    if(init_mode & VMI_INIT_EVENTS){
        events_init(*vmi);
    }

error_exit:
    return status;
}

status_t
vmi_init(
    vmi_instance_t *vmi,
    uint32_t flags,
    char *name)
{
    return vmi_init_private(vmi, flags | VMI_CONFIG_GLOBAL_FILE_ENTRY, VMI_INVALID_DOMID, name, NULL);
}

status_t
vmi_init_custom(
    vmi_instance_t *vmi,
    uint32_t flags,
    vmi_config_t config)
{
    status_t ret = VMI_FAILURE;
    uint32_t config_mode = flags & 0xFF000000;

    if (NULL == config) {
        config_mode |= VMI_CONFIG_NONE;
    }

    if (VMI_CONFIG_GLOBAL_FILE_ENTRY == config_mode) {

        ret = vmi_init(vmi, flags, (char *)config);
        goto _done;

    } else if (VMI_CONFIG_STRING == config_mode) {
        char *name = NULL;

        if (VMI_FILE == (*vmi)->mode) {
            name = strdup((*vmi)->image_type_complete);
        } else {
            name = strdup((*vmi)->image_type);
        }

        ret = vmi_init_private(vmi, flags, VMI_INVALID_DOMID, name,
                (vmi_config_t)config);

    } else if (VMI_CONFIG_GHASHTABLE == config_mode) {

        char *name = NULL;
        unsigned long domid = VMI_INVALID_DOMID;
        GHashTable *configtbl = (GHashTable *)config;
        gpointer idptr = NULL;

        name = (char *)g_hash_table_lookup(configtbl, "name");
        if(g_hash_table_lookup_extended(configtbl, "domid", NULL, &idptr)) {
            domid = *(unsigned long *)idptr;
        }

        if (name != NULL && domid != VMI_INVALID_DOMID) {
            errprint("--specifying both the name and domid is not supported\n");
        } else if (name != NULL) {
            ret = vmi_init_private(vmi, flags, VMI_INVALID_DOMID, name, config);
        } else if (domid != VMI_INVALID_DOMID) {
            ret = vmi_init_private(vmi, flags, domid, NULL, config);
        } else {
            errprint("--you need to specify either the name or the domid\n");
        }

        goto _done;

    } else {
        errprint("Custom configuration input type not defined!\n");
    }

_done:
    return ret;
}

status_t
vmi_init_complete(
    vmi_instance_t *vmi,
    char *config)
{
    uint32_t flags = VMI_INIT_COMPLETE | (*vmi)->mode;

    char *name = NULL;

    if (VMI_FILE == (*vmi)->mode) {
        name = strdup((*vmi)->image_type_complete);
    }
    else {
        name = strdup((*vmi)->image_type);
    }


    if(config) {
        flags |= VMI_CONFIG_STRING;
    } else if(name && ((*vmi)->config_mode & VMI_CONFIG_GLOBAL_FILE_ENTRY)) {
        flags |= VMI_CONFIG_GLOBAL_FILE_ENTRY;
    } else {
        flags |= VMI_CONFIG_NONE;
    }

    if (((*vmi)->flags) & VMI_INIT_EVENTS) {
        flags |= VMI_INIT_EVENTS;
    }

    vmi_destroy(*vmi);
    return vmi_init_private(vmi,
                            flags,
                            VMI_INVALID_DOMID,
                            name,
                            (vmi_config_t)config);
}

status_t
vmi_init_complete_custom(
    vmi_instance_t *vmi,
    uint32_t flags,
    vmi_config_t config)
{
    flags |= VMI_INIT_COMPLETE | (*vmi)->mode;
    vmi_destroy(*vmi);
    return vmi_init_custom(vmi, flags, config);
}

status_t
vmi_destroy(
    vmi_instance_t vmi)
{
    vmi->shutting_down = TRUE;
    if(vmi->init_mode & VMI_INIT_EVENTS){
        events_destroy(vmi);
    }
    driver_destroy(vmi);
    if (vmi->os_interface) {
        os_destroy(vmi);
    }
    if (vmi->os_data) {
        free(vmi->os_data);
    }
    if (vmi->arch_interface) {
        free(vmi->arch_interface);
    }
    vmi->os_data = NULL;
    pid_cache_destroy(vmi);
    sym_cache_destroy(vmi);
    rva_cache_destroy(vmi);
    v2p_cache_destroy(vmi);
#if ENABLE_SHM_SNAPSHOT == 1
    v2m_cache_destroy(vmi);
#endif
    memory_cache_destroy(vmi);
    if (vmi->image_type)
        free(vmi->image_type);
    if (vmi)
        free(vmi);
    return VMI_SUCCESS;
}
