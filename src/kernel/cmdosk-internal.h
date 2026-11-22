#ifndef CMDOSK_INTERNAL_H
#define CMDOSK_INTERNAL_H

#include "cmdosk-abi.h"

void tui_text_span(unsigned char x, unsigned char y, unsigned char width,
                   unsigned char style, const unsigned char* text);
void tui_box(unsigned char x, unsigned char y, unsigned char width,
             unsigned char height, unsigned char style);
void cmdosk_pick_file(void);
void cmdosk_menu(void);
void cmdosk_disk_dispatch(void);
void cmdosk_disk_finish(void);
unsigned char cmdosk_disk_busy(void);
unsigned char cmdosk_disk_status(void);
void cmdosk_copy(void);

#endif
