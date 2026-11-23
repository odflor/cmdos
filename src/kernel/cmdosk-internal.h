#ifndef CMDOSK_INTERNAL_H
#define CMDOSK_INTERNAL_H

#include "cmdosk-abi.h"
#include "disk-api.h"

void krnl_tui_text_span(unsigned char x, unsigned char y, unsigned char width,
                   unsigned char style, const unsigned char* text);
void krnl_tui_box(unsigned char x, unsigned char y, unsigned char width,
             unsigned char height, unsigned char style);
void krnl_tui_pick_file(void);
void krnl_tui_menu(void);
void krnl_disk_dispatch(void);
void krnl_disk_finish(void);
unsigned char krnl_disk_busy(void);
unsigned char krnl_disk_status(void);
void krnl_disk_copy(void);

#endif
