#ifndef CMDOSK_ABI_H
#define CMDOSK_ABI_H

#include <c128.h>

#define KRNL_TUI_BACKGROUND COLOR_BLACK
#define KRNL_TUI_TEXT COLOR_CYAN
#define KRNL_TUI_ACCENT COLOR_CYAN
#define KRNL_TUI_POPUP COLOR_YELLOW
#define KRNL_TUI_WIDTH 80
#define KRNL_ABI_VERSION 1
#define KRNL_BUFFER_SIZE 128

/* Byte-oriented ABI. No pointers into an application's bank cross the gate. */
#define KRNL_REQUEST (*(struct cmdosk_request*)0x0F70)
struct cmdosk_request {
    unsigned char operation, x, y, style, count, result, device;
    unsigned char data[KRNL_BUFFER_SIZE];
};

enum cmdosk_operation {
    KRNL_TUI_CLEAR       = 0x20,
    KRNL_TUI_DRAW        = 0x21,
    KRNL_TUI_CURSOR      = 0x22,
    KRNL_TUI_DIALOG      = 0x23,
    KRNL_TUI_BOX         = 0x24,
    KRNL_TUI_PICK_FILE   = 0x25,
    KRNL_TUI_MENU        = 0x26,
    KRNL_KEY_READ        = 0x30,
    KRNL_SESSION_KEY     = 0x31,
    KRNL_DISK_READ_OPEN  = 0x40,
    KRNL_DISK_WRITE_OPEN = 0x41,
    KRNL_DISK_READ       = 0x42,
    KRNL_DISK_WRITE      = 0x43,
    KRNL_DISK_CLOSE      = 0x44,
    KRNL_DISK_COMMAND    = 0x45,
    KRNL_COPY            = 0x47,
    KRNL_REU_STATUS      = 0x50,
    KRNL_ARGS            = 0x60
};
/* SESSION_KEY returns the key in result and the session event in count. */
#define KRNL_SESSION_RESUMED 0x10
#define KRNL_PICK_SELECTED 0x20
/* OPEN takes the device in y, style (CBM_T_*), and a filename in data.
 * On success x contains the handle. READ, WRITE, and CLOSE take the handle
 * in x. CLOSE with nonzero count aborts publication of a write to M:. */
#define KRNL_DISK_NOT_FOUND 62
#define KRNL_DISK_ERROR 0xFE

enum krnl_reu_result {
    KRNL_REU_ABSENT = 0x50,
    KRNL_REU_PRESENT = 0x51,
    KRNL_REU_16MIB = 0x52
};

/* KRNL_COPY: x and y are source and destination devices (0 means M:).
 * data holds two NUL-terminated names at offsets 0 and 17. */
#define KRNL_COPY_DESTINATION 17
enum cmdosk_copy_result {
    KRNL_COPY_OK = 0x30, KRNL_COPY_INVALID, KRNL_COPY_UNMOUNTED, KRNL_COPY_SOURCE,
    KRNL_COPY_TYPE, KRNL_COPY_DEST, KRNL_COPY_IO, KRNL_COPY_PARTIAL,
    KRNL_COPY_UNSUPPORTED = 0x3F
};

/* KRNL_TUI_MENU: y is top row, style is outer width, count is payload size.
 * Payload: group count, then for each group x, item count, and a
 * NUL-terminated PETSCII label per item. Result is the 1-based item
 * index across all groups, or zero on cancel or invalid input.
 * The kernel clears the menu footprint on exit. The caller repaints it. */
extern unsigned char reu_detected;
extern unsigned char mounted;

/* KRNL_ARGS returns the NUL-terminated PETSCII argument tail in data. */
extern const char* cmdp_arguments;

void krnl_tui_reset(void);
void krnl_service_dispatch(void);
unsigned char krnl_disk_send_command(unsigned char device, const char* command);
void krnl_keyboard_editor_keys(unsigned char enable);
#define KRNL_BANK1_COMMON_SAVE_SIZE 0x1F7
void krnl_bank1_prepare(unsigned char device);
void krnl_bank1_finish(void);
void krnl_bank1_store(unsigned int address);
int krnl_bank1_run(unsigned int address);

#endif
