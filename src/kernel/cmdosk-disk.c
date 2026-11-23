#include <cbm.h>
#include <conio.h>
#include <string.h>
#include "cmdosk-internal.h"
#include "reu.h"
#include "filename.h"

#define STATUS_LFN 15
#define KERNAL_OPEN_FILES (*(volatile unsigned char*)0x98)
#define KERNAL_FILE_DEVICES ((volatile const unsigned char*)0x036C)

#define FIRST_LFN 4
#define CHANNELS 2
typedef struct {
    unsigned char mode, device, slot, type;
    unsigned int position, size;
    char name[17];
} Channel;

static Channel channels[CHANNELS];

static void reu_record(unsigned char slot, unsigned char size)
{
    reu_buffer = reu_dir_name;
    reu_length = size;
    krnl_reu_write_entry(slot - 1);
}

void krnl_reu_publish(unsigned char slot, const char* name,
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

    if (command[1] != '0' || command[2] != ':') return 255;
    if (*command == 's') {
        if (!*name || !(slot = krnl_reu_find(name))) return 62;
        *reu_dir_name = 0;
        reu_record(slot, 1);
        return 1;
    }
    if (*command != 'r' || !(equals = strchr(name, '=')) ||
        equals[1] != '0' || equals[2] != ':' || !equals[3]) return 255;
    *equals = 0;
    if (!*name || strlen(name) > 16 || krnl_reu_find(name)) return 63;
    slot = krnl_reu_find(equals + 3);
    if (!slot) return 62;
    strncpy(reu_dir_name, name, 16);
    reu_record(slot, 16);
    return 0;
}

unsigned char krnl_disk_status(void)
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

unsigned char krnl_disk_send_command(unsigned char device, const char* command)
{
    unsigned char result = 255;

    if (device && cbm_open(STATUS_LFN, device, 15, command) == 0) {
        result = krnl_disk_status();
        cbm_k_close(STATUS_LFN);
    }
    return result;
}

unsigned char krnl_file_open(unsigned char device, const char* name,
                             unsigned char type, unsigned char write,
                             unsigned char* result)
{
    unsigned char i, slot, handle;
    Channel* ch;
    char request[24], suffix[] = ",p,r";

    *result = KRNL_DISK_ERROR;
    if (!krnl_valid_filename(name) || (device && (device < 8 || device > 11))) return 0;
    for (handle = 0; handle < CHANNELS && channels[handle].mode; ++handle) {}
    if (handle == CHANNELS) return 0;
    ch = &channels[handle];
    if (type == CBM_T_SEQ) suffix[1] = 's';
    else if (type == CBM_T_USR) suffix[1] = 'u';
    else if (type != CBM_T_PRG) return 0;
    if (device) {
        if (write) suffix[3] = 'w';
        strcpy(request, "0:");
        strcat(request, name);
        strcat(request, suffix);
        if (cbm_open(STATUS_LFN, device, 15, "") != 0) return 0;
        i = cbm_open(FIRST_LFN + handle, device, FIRST_LFN + handle, request);
        *result = krnl_disk_status();
        cbm_close(STATUS_LFN);
        if (i || *result) {
            cbm_close(FIRST_LFN + handle);
            if (!*result) *result = KRNL_DISK_ERROR;
            return 0;
        }
    } else {
        if (!mounted) return 0;
        slot = krnl_reu_find(name);
        if (write) {
            if (slot) return 0;
            for (i = 0; i < CHANNELS; ++i) {
                if (channels[i].mode == 2 && !channels[i].device &&
                    !strcmp(name, channels[i].name)) return 0;
            }
            for (slot = 1; slot <= 16; ++slot) {
                krnl_reu_dir_read(slot - 1);
                if (*reu_dir_name) continue;
                for (i = 0; i < CHANNELS; ++i) {
                    if (channels[i].mode && !channels[i].device && channels[i].slot == slot) break;
                }
                if (i == CHANNELS) break;
            }
            if (slot > 16) return 0;
            strcpy(ch->name, name);
        } else {
            if (!slot) {
                *result = KRNL_DISK_NOT_FOUND;
                return 0;
            }
            if (reu_dir_type != type) return 0;
            ch->size = reu_dir_size;
        }
        ch->slot = slot;
    }
    ch->device = device;
    ch->type = type;
    if (device) ch->size = 0;
    ch->position = 0;
    ch->mode = write ? 2 : 1;
    *result = 0;
    return handle + 1;
}

int krnl_file_transfer(unsigned char handle, void* data, unsigned char count,
                       unsigned char write)
{
    Channel* ch;
    int transferred;

    if (!handle || handle > CHANNELS || count > KRNL_BUFFER_SIZE) return -1;
    ch = &channels[handle - 1];
    if (ch->mode != (write ? 2 : 1)) return -1;
    if (!count) return 0;
    if (ch->device) {
        if (!write && ch->size) return 0;
        handle += FIRST_LFN - 1;
        transferred = write ? cbm_write(handle, data, count) : cbm_read(handle, data, count);
        if (cbm_k_readst() & (write ? 0xff : 0xbf)) return -1;
        ch->size = cbm_k_readst() & 0x40;
        return transferred;
    }
    if (write) {
        if (count > 0xffffu - ch->position) return -1;
    } else if (count > ch->size - ch->position) {
        count = ch->size - ch->position;
    }
    if (!count) return 0;
    reu_offset[2] = ch->slot;
    reu_position = ch->position;
    reu_buffer = data;
    reu_length = count;
    krnl_reu_transfer(write ? 0x90 : 0x91);
    ch->position += count;
    return count;
}

unsigned char krnl_file_close(unsigned char handle, unsigned char abort)
{
    Channel* ch;
    unsigned char status = 0;

    if (!handle || handle > CHANNELS) return KRNL_DISK_ERROR;
    ch = &channels[handle - 1];
    if (!ch->mode) return KRNL_DISK_ERROR;
    if (ch->device) {
        unsigned char check_status = ch->mode == 2 && !abort;
        if (check_status) status = cbm_open(STATUS_LFN, ch->device, 15, "");
        cbm_close(FIRST_LFN + handle - 1);
        if (check_status && !status) {
            status = krnl_disk_status();
            cbm_close(STATUS_LFN);
        }
    } else if (ch->mode == 2 && !abort) {
        krnl_reu_publish(ch->slot, ch->name, ch->position, ch->type);
    }
    ch->mode = 0;
    return status;
}

void krnl_disk_finish(void)
{
    unsigned char i;
    for (i = 1; i <= CHANNELS; ++i) krnl_file_close(i, 1);
}

unsigned char krnl_disk_busy(void)
{
    unsigned char i;

    for (i = 0; i < CHANNELS; ++i) if (channels[i].mode) return 1;
    for (i = 0; i < KERNAL_OPEN_FILES && i < 10; ++i) {
        if (KERNAL_FILE_DEVICES[i] >= 8) return 1;
    }
    return 0;
}

void krnl_disk_dispatch(void)
{
    int count;

    switch (KRNL_REQUEST.operation) {
        case KRNL_COPY:
            KRNL_REQUEST.count = 0;
            if (krnl_disk_busy()) KRNL_REQUEST.result = KRNL_COPY_IO;
            else krnl_disk_copy();
            break;
        case KRNL_REU_STATUS:
            KRNL_REQUEST.result = reu_detected == 2 ? KRNL_REU_16MIB
                : (reu_detected ? KRNL_REU_PRESENT : KRNL_REU_ABSENT);
            break;
        case KRNL_DISK_COMMAND:
            if (krnl_disk_busy()) {
                KRNL_REQUEST.result = KRNL_DISK_ERROR;
                break;
            }
            KRNL_REQUEST.result = KRNL_REQUEST.device
                ? krnl_disk_send_command(KRNL_REQUEST.device, (char*)KRNL_REQUEST.data)
                : reu_disk_command((char*)KRNL_REQUEST.data);
            break;
        case KRNL_TUI_PICK_FILE:
            if (!krnl_disk_busy()) {
                krnl_tui_pick_file();
            }
            break;
        case KRNL_DISK_READ_OPEN:
        case KRNL_DISK_WRITE_OPEN:
            KRNL_REQUEST.data[16] = 0;
            KRNL_REQUEST.x = krnl_file_open(KRNL_REQUEST.y,
                (const char*)KRNL_REQUEST.data, KRNL_REQUEST.style,
                KRNL_REQUEST.operation == KRNL_DISK_WRITE_OPEN, &KRNL_REQUEST.result);
            break;
        case KRNL_DISK_READ:
        case KRNL_DISK_WRITE:
            count = krnl_file_transfer(KRNL_REQUEST.x, KRNL_REQUEST.data, KRNL_REQUEST.count,
                                      KRNL_REQUEST.operation == KRNL_DISK_WRITE);
            if (KRNL_REQUEST.operation == KRNL_DISK_WRITE)
                KRNL_REQUEST.result = count == KRNL_REQUEST.count ? 0 : KRNL_DISK_ERROR;
            else
                KRNL_REQUEST.result = count < 0 ? KRNL_DISK_ERROR : (unsigned char)count;
            break;
        case KRNL_DISK_CLOSE:
            KRNL_REQUEST.result = krnl_file_close(KRNL_REQUEST.x, KRNL_REQUEST.count);
            break;
    }
}
