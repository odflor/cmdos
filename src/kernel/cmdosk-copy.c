#include <cbm.h>
#include <string.h>
#include "cmdosk-internal.h"
#include "filename.h"
#include "reu.h"

#define SRC_LFN 2

typedef struct {
    unsigned char device;
    const char* name;
} Path;

static unsigned char buffer[KRNL_BUFFER_SIZE];

static unsigned char valid_device(unsigned char device)
{
    return !device || (device >= 8 && device <= 11);
}

/* 1 found, 0 missing, -1 directory unavailable or malformed. */
static int lookup_iec(const Path* path, unsigned char* type)
{
    struct cbm_dirent entry;
    unsigned char result;
    int found = 0;

    if (cbm_opendir(SRC_LFN, path->device)) return -1;
    while ((result = cbm_readdir(SRC_LFN, &entry)) == 0) {
        entry.name[KRNL_FILENAME_MAX] = 0;
        if (entry.type != CBM_T_HEADER && strcmp(entry.name, path->name) == 0) {
            *type = entry.type;
            found = 1;
            break;
        }
    }
    cbm_closedir(SRC_LFN);
    return found || result == 2 ? found : -1;
}

void krnl_disk_copy(void)
{
    Path src, dst;
    char request[20];
    unsigned char type, ignored, src_handle, dst_handle, status;
    int count;

    KRNL_REQUEST.result = KRNL_COPY_INVALID;
    src.device = KRNL_REQUEST.x;
    dst.device = KRNL_REQUEST.y;
    src.name = (char*)KRNL_REQUEST.data;
    dst.name = src.name + KRNL_COPY_DESTINATION;
    KRNL_REQUEST.data[16] = 0;
    KRNL_REQUEST.data[KRNL_COPY_DESTINATION + 16] = 0;
    if (!valid_device(src.device) || !valid_device(dst.device) ||
        !krnl_valid_filename(src.name) || !krnl_valid_filename(dst.name)) return;

    if ((!src.device || !dst.device) && !mounted) {
        KRNL_REQUEST.result = KRNL_COPY_UNMOUNTED;
        return;
    }
    KRNL_REQUEST.result = KRNL_COPY_SOURCE;
    if (src.device) {
        if (lookup_iec(&src, &type) != 1) return;
    } else {
        if (!krnl_reu_find(src.name)) return;
        type = reu_dir_type;
    }
    KRNL_REQUEST.result = KRNL_COPY_TYPE;
    if (type != CBM_T_PRG && type != CBM_T_SEQ && type != CBM_T_USR) return;
    KRNL_REQUEST.result = KRNL_COPY_DEST;
    if (dst.device && lookup_iec(&dst, &ignored) != 0) return;

    KRNL_REQUEST.result = KRNL_COPY_SOURCE;
    src_handle = krnl_file_open(src.device, src.name, type, 0, &status);
    if (!src_handle) return;
    dst_handle = krnl_file_open(dst.device, dst.name, type, 1, &status);
    if (!dst_handle) {
        krnl_file_close(src_handle, 0);
        KRNL_REQUEST.result = KRNL_COPY_DEST;
        return;
    }

    while ((count = krnl_file_transfer(src_handle, buffer, sizeof(buffer), 0)) > 0) {
        if (krnl_file_transfer(dst_handle, buffer, count, 1) != count) {
            count = -1;
            break;
        }
    }
    krnl_file_close(src_handle, 0);
    status = krnl_file_close(dst_handle, count < 0);
    if (!status && count == 0) {
        KRNL_REQUEST.result = KRNL_COPY_OK;
        return;
    }
    KRNL_REQUEST.result = KRNL_COPY_IO;
    if (dst.device) {
        strcpy(request, "s0:");
        strcat(request, dst.name);
        if (krnl_disk_send_command(dst.device, request) != 1)
            KRNL_REQUEST.result = KRNL_COPY_PARTIAL;
    }
}
