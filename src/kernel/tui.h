#ifndef CMDOS_TUI_H
#define CMDOS_TUI_H

#include <c128.h>

#define TUI_BACKGROUND COLOR_BLACK
#define TUI_TEXT COLOR_CYAN
#define TUI_ACCENT COLOR_CYAN
#define TUI_POPUP COLOR_YELLOW
#define TUI_WIDTH 80
#define TUI_TRANSFER 128

/* Byte-oriented ABI. No pointers into an application's bank cross the gate. */
#define TUI_REQUEST (*(struct tui_request*)0x0F70)
struct tui_request {
    unsigned char operation, x, y, style, count, result, device;
    unsigned char data[TUI_TRANSFER];
};

enum tui_operation {
    TUI_CLEAR, TUI_DRAW, TUI_KEY, TUI_READ_OPEN, TUI_WRITE_OPEN,
    TUI_READ, TUI_WRITE, TUI_CLOSE, TUI_CURSOR, TUI_DIALOG,
    TUI_BOX, TUI_DISK_COMMAND, TUI_PICK_FILE
};

void tui_reset(void);
void tui_dispatch(void);
void tui_editor_keys(unsigned char enable);
void bank1_prepare(unsigned char device);
void bank1_finish(void);
void bank1_store(unsigned int address);
void bank1_run(unsigned int address);

#endif
