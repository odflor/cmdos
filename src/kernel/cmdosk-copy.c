#include <cbm.h>
#include <string.h>
#include "cmdosk-internal.h"
#include "filename.h"
#include "reu.h"
#include "cmdosk-swap.h"

#define SRC_LFN 2
#define DST_LFN 3
#define STATUS_LFN 15

typedef struct {
    unsigned char device;
    const char* name;
} Path;

static unsigned char buffer[TUI_TRANSFER];

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
        entry.name[CMDOS_FILENAME_MAX] = 0;
        if (entry.type != CBM_T_HEADER && strcmp(entry.name, path->name) == 0) {
            *type = entry.type;
            found = 1;
            break;
        }
    }
    cbm_closedir(SRC_LFN);
    return found || result == 2 ? found : -1;
}

static void iec_name(char* request, const char* name, char type, char mode)
{
    char suffix[] = ",p,r";
    suffix[1] = type;
    suffix[3] = mode;
    strcpy(request, "0:");
    strcat(request, name);
    strcat(request, suffix);
}

static void transfer(unsigned char slot, unsigned int offset,
                     unsigned char count, unsigned char operation)
{
    reu_buffer = buffer;
    reu_position = offset;
    reu_offset[2] = slot;
    reu_length = count;
    reu_transfer(operation);
}

void cmdosk_copy(void)
{
    Path src, dst;
    char request[24];
    char kind = 'p';
    unsigned char src_slot = 0, dst_slot = 0, type, status, ignored;
    unsigned char src_open = 0, dst_open = 0, status_open = 0, created = 0;
    unsigned int remaining = 0;
    unsigned long copied = 0;
    unsigned char ok = 0;
    int count;

    TUI_REQUEST.result = TUI_COPY_INVALID;
    src.device = TUI_REQUEST.x;
    dst.device = TUI_REQUEST.y;
    src.name = (char*)TUI_REQUEST.data;
    dst.name = src.name + TUI_COPY_DESTINATION;
    TUI_REQUEST.data[16] = 0;
    TUI_REQUEST.data[TUI_COPY_DESTINATION + 16] = 0;
    if (!valid_device(src.device) || !valid_device(dst.device) ||
        !valid_filename(src.name) || !valid_filename(dst.name)) return;

    if ((!src.device || !dst.device) && !mounted) {
        TUI_REQUEST.result = TUI_COPY_UNMOUNTED;
        return;
    }
    if (src.device) {
        if (lookup_iec(&src, &type) != 1) goto source_error;
    } else {
        src_slot = reu_find(src.name);
        if (!src_slot) goto source_error;
        remaining = reu_dir_size;
        type = reu_dir_type;
    }
    if (type == CMDOSK_SWAP_TYPE) {
        TUI_REQUEST.result = TUI_COPY_TYPE;
        return;
    }
    if (type == CBM_T_SEQ) kind = 's';
    else if (type == CBM_T_USR) kind = 'u';
    else if (type != CBM_T_PRG && (src.device || dst.device)) {
        TUI_REQUEST.result = TUI_COPY_TYPE;
        return;
    }
    if (dst.device) {
        if (lookup_iec(&dst, &ignored) != 0) goto destination_error;
    } else {
        if (reu_find(dst.name) || !(dst_slot = reu_find("")))
            goto destination_error;
    }

    if (src.device) {
        iec_name(request, src.name, kind, 'r');
        if (cbm_open(SRC_LFN, src.device, SRC_LFN, request)) goto done;
        src_open = 1;
    }
    if (dst.device) {
        if (cbm_open(STATUS_LFN, dst.device, 15, "")) goto done;
        status_open = 1;
        iec_name(request, dst.name, kind, 'w');
        if (cbm_open(DST_LFN, dst.device, DST_LFN, request)) goto done;
        dst_open = 1;
        if (cmdosk_disk_status()) goto done;
        created = 1;
    }
    for (;;) {
        status = 0;
        if (src.device) {
            count = cbm_read(SRC_LFN, buffer, sizeof(buffer));
            status = cbm_k_readst();
            if (count < 0 || (status & 0xbf)) goto done;
        } else {
            count = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
            transfer(src_slot, (unsigned int)copied, count, 0x91);
            remaining -= count;
        }
        if (!count) break;
        if (dst.device) {
            if (cbm_write(DST_LFN, buffer, count) != count || cbm_k_readst()) goto done;
        } else {
            if (copied + count > 65535ul) goto done;
            transfer(dst_slot, (unsigned int)copied, count, 0x90);
        }
        copied += count;
        if (status & 0x40) break;
    }
    ok = 1;

done:
    if (src_open) cbm_close(SRC_LFN);
    if (dst_open) cbm_close(DST_LFN);
    if (status_open) {
        if (cmdosk_disk_status()) ok = 0;
        cbm_close(STATUS_LFN);
    }
    if (ok) {
        if (!dst.device) cmdosk_reu_publish(dst_slot, dst.name, (unsigned int)copied, type);
        TUI_REQUEST.result = TUI_COPY_OK;
        return;
    }
    TUI_REQUEST.result = TUI_COPY_IO;
    if (created) {
        strcpy(request, "s0:");
        strcat(request, dst.name);
        if (cmdosk_disk_command(dst.device, request) != 1)
            TUI_REQUEST.result = TUI_COPY_PARTIAL;
    }
    return;

source_error:
    TUI_REQUEST.result = TUI_COPY_SOURCE;
    return;
destination_error:
    TUI_REQUEST.result = TUI_COPY_DEST;
}
