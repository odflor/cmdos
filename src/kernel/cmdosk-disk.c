#include <cbm.h>
#include <conio.h>
#include <string.h>
#include "cmdosk-internal.h"
#include "reu.h"
#include "disk-api.h"

#define FILE_LFN 4
#define STATUS_LFN 15

static unsigned char file_open;
static char reu_write_name[17];

static void reu_record(unsigned char slot, unsigned char size)
{
    reu_buffer = reu_dir_name;
    reu_length = size;
    reu_write_entry(slot - 1);
}

void cmdosk_reu_publish(unsigned char slot, const char* name,
                       unsigned int size, unsigned char type)
{
    strncpy(reu_dir_name, name, 16);
    reu_dir_name[16] = (unsigned char)size;
    reu_dir_name[17] = (unsigned char)(size >> 8);
    reu_dir_name[18] = type;
    reu_record(slot, 19);
}

static unsigned char reu_disk_command(char* command)
{
    char* name = command + 3;
    char* equals;
    unsigned char slot;

    if (file_open || command[1] != '0' || command[2] != ':') return 255;
    if (*command == 's') {
        if (!*name || !(slot = reu_find(name))) return 62;
        *reu_dir_name = 0;
        reu_record(slot, 1);
        return 1;
    }
    if (*command != 'r' || !(equals = strchr(name, '=')) ||
        equals[1] != '0' || equals[2] != ':' || !equals[3]) return 255;
    *equals = 0;
    if (!*name || strlen(name) > 16 || reu_find(name)) return 63;
    slot = reu_find(equals + 3);
    if (!slot) return 62;
    strncpy(reu_dir_name, name, 16);
    reu_record(slot, 16);
    return 0;
}

static unsigned char reu_open(char* name, unsigned char write)
{
    unsigned char slot;
    if (!*name || strlen(name) > 16) return 1;
    slot = reu_find(name);
    if (write) {
        if (slot) return 1;
        slot = reu_find("");
        if (!slot) return 1;
        strcpy(reu_write_name, name);
    } else if (!slot || reu_dir_type != CBM_T_SEQ) {
        return 1;
    }
    reu_offset[2] = slot;
    reu_position = 0;
    file_open = write ? 2 : 1;
    return 0;
}

unsigned char cmdosk_disk_status(void)
{
    unsigned char first, second, status = 255;

    if (cbm_k_chkin(STATUS_LFN) == 0) {
        first = cbm_k_basin() - '0';
        second = cbm_k_basin() - '0';
        if (first <= 9 && second <= 9) {
            status = first * 10 + second;
        }
        while (!cbm_k_readst() && cbm_k_basin() != CH_ENTER) {
        }
    }
    cbm_k_clrch();
    return status;
}

unsigned char cmdosk_disk_command(unsigned char device, const char* command)
{
    unsigned char result = 255;

    if (device && !file_open && cbm_open(STATUS_LFN, device, 15, command) == 0) {
        result = cmdosk_disk_status();
        cbm_k_close(STATUS_LFN);
    }
    return result;
}

static const struct disk_api disk_api = {
    cbm_open, cbm_close, cbm_read, cbm_write, cbm_opendir, cbm_readdir,
    cbm_k_readst, cmdosk_disk_status, reu_dir_read, reu_write_entry, reu_transfer,
    reu_dir_name, &reu_dir_size, &reu_dir_type, &reu_buffer, &reu_length,
    &reu_position, reu_offset
};

void cmdosk_disk_finish(void)
{
    if (file_open && TUI_REQUEST.device) {
        cbm_k_close(FILE_LFN);
        cbm_k_close(STATUS_LFN);
    }
    file_open = 0;
}

unsigned char cmdosk_disk_busy(void)
{
    return file_open != 0;
}

void cmdosk_disk_dispatch(void)
{
    int count;

    switch (TUI_REQUEST.operation) {
        case TUI_COPY:
            TUI_REQUEST.count = 0;
            if (file_open) TUI_REQUEST.result = TUI_COPY_IO;
            else cmdosk_copy();
            break;
        case TUI_REU:
            TUI_REQUEST.result = reu_detected;
            break;
        case TUI_DISK_API:
            memcpy(TUI_REQUEST.data, &disk_api, sizeof(disk_api));
            TUI_REQUEST.result = 0x40 | mounted;
            break;
        case TUI_DISK_COMMAND:
            TUI_REQUEST.result = TUI_REQUEST.device
                ? cmdosk_disk_command(TUI_REQUEST.device, (char*)TUI_REQUEST.data)
                : reu_disk_command((char*)TUI_REQUEST.data);
            break;
        case TUI_PICK_FILE:
            if (!file_open) {
                cmdosk_pick_file();
            }
            break;
        case TUI_READ_OPEN:
        case TUI_WRITE_OPEN:
            if (!TUI_REQUEST.device) {
                TUI_REQUEST.result = 1;
                if (!file_open) {
                    char* name = (char*)TUI_REQUEST.data + 2;
                    char* suffix = strchr(name, ',');
                    if (!suffix || suffix == name) break;
                    *suffix = 0;
                    TUI_REQUEST.result = reu_open(name, TUI_REQUEST.operation == TUI_WRITE_OPEN);
                }
                break;
            }
            if (file_open || cbm_open(STATUS_LFN, TUI_REQUEST.device, 15, "") != 0) {
                TUI_REQUEST.result = 1;
                break;
            }
            if (cbm_open(FILE_LFN, TUI_REQUEST.device, CBM_SEQ,
                         (char*)TUI_REQUEST.data) != 0 || cmdosk_disk_status()) {
                cbm_k_close(FILE_LFN);
                cbm_k_close(STATUS_LFN);
                TUI_REQUEST.result = 1;
                break;
            }
            file_open = 1;
            break;
        case TUI_READ:
            if (file_open != 1 || TUI_REQUEST.count > TUI_TRANSFER) {
                TUI_REQUEST.result = 255;
                break;
            }
            count = TUI_REQUEST.device ? cbm_read(FILE_LFN, TUI_REQUEST.data, TUI_REQUEST.count)
                                       : reu_read(TUI_REQUEST.data, TUI_REQUEST.count);
            TUI_REQUEST.result = count < 0 ? 255 : (unsigned char)count;
            break;
        case TUI_WRITE:
            if (!file_open || TUI_REQUEST.count > TUI_TRANSFER) {
                TUI_REQUEST.result = 1;
                break;
            }
            if (!TUI_REQUEST.device) {
                if (file_open != 2 || TUI_REQUEST.count > 0xffffu - reu_position) {
                    file_open = 0;
                    TUI_REQUEST.result = 1;
                    break;
                }
                reu_buffer = TUI_REQUEST.data;
                reu_length = TUI_REQUEST.count;
                reu_transfer(0x90);
                reu_position += reu_length;
                break;
            }
            count = cbm_write(FILE_LFN, TUI_REQUEST.data, TUI_REQUEST.count);
            TUI_REQUEST.result = count != TUI_REQUEST.count;
            break;
        case TUI_CLOSE:
            if (!TUI_REQUEST.device) {
                if (file_open == 2) {
                    cmdosk_reu_publish(reu_offset[2], reu_write_name, reu_position, CBM_T_SEQ);
                }
                file_open = 0;
                break;
            }
            cbm_k_close(FILE_LFN);
            TUI_REQUEST.result = cmdosk_disk_status();
            cbm_k_close(STATUS_LFN);
            file_open = 0;
            break;
    }
}
