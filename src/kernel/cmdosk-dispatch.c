#include <conio.h>
#include <string.h>
#include "cmdosk-internal.h"
#include "cmdosk-context.h"

void krnl_service_dispatch(void)
{
    KRNL_REQUEST.result = 0;
    switch (KRNL_REQUEST.operation) {
        case KRNL_ARGS:
            strcpy((char*)KRNL_REQUEST.data, cmdp_arguments);
            break;
        case KRNL_TUI_CLEAR:
            krnl_tui_reset();
            cursor(0);
            break;
        case KRNL_TUI_DRAW:
            if (KRNL_REQUEST.y < 24 && KRNL_REQUEST.x < KRNL_TUI_WIDTH &&
                KRNL_REQUEST.count <= KRNL_TUI_WIDTH - KRNL_REQUEST.x) {
                krnl_tui_text_span(KRNL_REQUEST.x, KRNL_REQUEST.y, KRNL_REQUEST.count,
                          KRNL_REQUEST.style, KRNL_REQUEST.data);
            }
            break;
        case KRNL_TUI_DIALOG:
            krnl_tui_box(15, 8, 50, 5, KRNL_TUI_POPUP | 0x80);
            krnl_tui_text_span(16, 9, 48, KRNL_TUI_POPUP | 0x80, KRNL_REQUEST.data);
            krnl_tui_text_span(16, 10, 48, KRNL_TUI_POPUP | 0x80, KRNL_REQUEST.data + 50);
            break;
        case KRNL_TUI_BOX:
            if (KRNL_REQUEST.count >= 2 && KRNL_REQUEST.data[0] >= 2 &&
                KRNL_REQUEST.x + KRNL_REQUEST.count <= KRNL_TUI_WIDTH &&
                KRNL_REQUEST.y + KRNL_REQUEST.data[0] <= 24) {
                krnl_tui_box(KRNL_REQUEST.x, KRNL_REQUEST.y, KRNL_REQUEST.count,
                    KRNL_REQUEST.data[0], KRNL_REQUEST.style);
            }
            break;
        case KRNL_KEY_READ:
            KRNL_REQUEST.result = cgetc();
            break;
        case KRNL_SESSION_KEY:
            KRNL_REQUEST.count = 0;
            KRNL_REQUEST.result = cgetc();
            if (krnl_session_hotkey(KRNL_REQUEST.result)) {
                krnl_session_suspend();
                KRNL_REQUEST.count = KRNL_SESSION_RESUMED;
            }
            break;
        case KRNL_TUI_MENU:
            krnl_tui_menu();
            break;
        case KRNL_TUI_CURSOR:
            gotoxy(KRNL_REQUEST.x, KRNL_REQUEST.y);
            cursor(1);
            break;
        default:
            krnl_disk_dispatch();
            break;
    }
}
