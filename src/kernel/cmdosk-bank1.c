#include <conio.h>
#include <string.h>
#include "cmdosk-internal.h"

#define MMU_RAM (*(volatile unsigned char*)0xD506)
/* CMD-OS keeps the VIC-II blank and uses the VDC for its console. */
#define VIC_COMMON_SAVE ((unsigned char*)0x0400)
#define VIC_KEY_SAVE ((volatile unsigned char*)0xD800)
#define SAVED_KEYS_SIZE 18

extern unsigned char bank1_gate[], bank1_gate_size[];
static unsigned char saved_ram;

void tui_editor_keys(unsigned char enable)
{
    unsigned char* const keys = (unsigned char*)0x1000;
    unsigned char i;
    static const unsigned char raw_keys[] = {
        CH_F1, CH_F2, CH_F3, CH_F4, CH_F5, CH_F6, CH_F7, CH_F8
    };

    if (enable) {
        for (i = 0; i < SAVED_KEYS_SIZE; ++i) {
            VIC_KEY_SAVE[i] = keys[i] & 15;
            VIC_KEY_SAVE[i + SAVED_KEYS_SIZE] = keys[i] >> 4;
        }
        memset(keys, 1, sizeof(raw_keys));
        keys[8] = keys[9] = 0;
        memcpy(keys + 10, raw_keys, sizeof(raw_keys));
    } else {
        for (i = 0; i < SAVED_KEYS_SIZE; ++i) {
            keys[i] = (VIC_KEY_SAVE[i] & 15) |
                      ((VIC_KEY_SAVE[i + SAVED_KEYS_SIZE] & 15) << 4);
        }
    }
}

void bank1_prepare(unsigned char device)
{
    unsigned int i;
    unsigned char* const common = (unsigned char*)0x0E00;

    memcpy(VIC_COMMON_SAVE, common, TUI_COMMON_SAVE_SIZE);
    for (i = 0; i < (unsigned int)bank1_gate_size; ++i) {
        common[i] = bank1_gate[i];
    }
    saved_ram = MMU_RAM;
    MMU_RAM = (saved_ram & 0xF0) | 0x05; /* Bottom 4 KiB common. */
    TUI_REQUEST.device = device;
}

void bank1_finish(void)
{
    unsigned char* const common = (unsigned char*)0x0E00;

    cmdosk_disk_finish();
    MMU_RAM = saved_ram;
    memcpy(common, VIC_COMMON_SAVE, TUI_COMMON_SAVE_SIZE);
}
