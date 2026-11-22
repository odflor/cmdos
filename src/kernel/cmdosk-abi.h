#ifndef CMDOSK_ABI_H
#define CMDOSK_ABI_H

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
    TUI_BOX, TUI_DISK_COMMAND, TUI_PICK_FILE, TUI_ARGUMENTS, TUI_REU,
    TUI_DISK_API, TUI_MENU, TUI_COPY, TUI_SESSION_KEY
};
#define TUI_SESSION_RESUMED 1

/* TUI_COPY: x and y are source and destination devices (0 means M:).
 * data holds two NUL-terminated names at offsets 0 and 17. */
#define TUI_COPY_DESTINATION 17
enum tui_copy_result {
    TUI_COPY_OK = 0, TUI_COPY_INVALID, TUI_COPY_UNMOUNTED, TUI_COPY_SOURCE,
    TUI_COPY_TYPE, TUI_COPY_DEST, TUI_COPY_IO, TUI_COPY_PARTIAL,
    TUI_COPY_UNSUPPORTED = 255
};

/* TUI_MENU: y is top row, style is outer width, count is payload size.
 * Payload: group count, then for each group x, item count, and a
 * NUL-terminated PETSCII label per item. Result is the 1-based item
 * index across all groups, or zero on cancel or invalid input.
 * The kernel clears the menu footprint on exit. The caller repaints it. */
extern unsigned char reu_detected;
extern unsigned char mounted;

/* TUI_ARGUMENTS returns the NUL-terminated PETSCII argument tail in data. */
extern const char* cmdp_arguments;

void tui_reset(void);
void cmdosk_dispatch(void);
unsigned char cmdosk_disk_command(unsigned char device, const char* command);
void tui_editor_keys(unsigned char enable);
#define TUI_COMMON_SAVE_SIZE 0x1F7
void bank1_prepare(unsigned char device);
void bank1_finish(void);
void bank1_store(unsigned int address);
int bank1_run(unsigned int address);

#endif
