#include <conio.h>
#include <string.h>
#include "cmdosk-internal.h"
#include "cmdosk-context.h"

void cmdosk_dispatch(void)
{
    TUI_REQUEST.result = 0;
    switch (TUI_REQUEST.operation) {
        case TUI_ARGUMENTS:
            strcpy((char*)TUI_REQUEST.data, cmdp_arguments);
            break;
        case TUI_CLEAR:
            tui_reset();
            cursor(0);
            break;
        case TUI_DRAW:
            if (TUI_REQUEST.y < 24 && TUI_REQUEST.x < TUI_WIDTH &&
                TUI_REQUEST.count <= TUI_WIDTH - TUI_REQUEST.x) {
                tui_text_span(TUI_REQUEST.x, TUI_REQUEST.y, TUI_REQUEST.count,
                          TUI_REQUEST.style, TUI_REQUEST.data);
            }
            break;
        case TUI_DIALOG:
            tui_box(15, 8, 50, 5, TUI_POPUP | 0x80);
            tui_text_span(16, 9, 48, TUI_POPUP | 0x80, TUI_REQUEST.data);
            tui_text_span(16, 10, 48, TUI_POPUP | 0x80, TUI_REQUEST.data + 50);
            break;
        case TUI_BOX:
            if (TUI_REQUEST.count >= 2 && TUI_REQUEST.data[0] >= 2 &&
                TUI_REQUEST.x + TUI_REQUEST.count <= TUI_WIDTH &&
                TUI_REQUEST.y + TUI_REQUEST.data[0] <= 24) {
                tui_box(TUI_REQUEST.x, TUI_REQUEST.y, TUI_REQUEST.count,
                    TUI_REQUEST.data[0], TUI_REQUEST.style);
            }
            break;
        case TUI_KEY:
            TUI_REQUEST.result = cgetc();
            break;
        case TUI_SESSION_KEY:
            TUI_REQUEST.result = cgetc();
            if (cmdosk_session_hotkey(TUI_REQUEST.result)) {
                cmdosk_session_suspend();
                TUI_REQUEST.result = TUI_SESSION_RESUMED;
            }
            break;
        case TUI_MENU:
            cmdosk_menu();
            break;
        case TUI_CURSOR:
            gotoxy(TUI_REQUEST.x, TUI_REQUEST.y);
            cursor(1);
            break;
        default:
            cmdosk_disk_dispatch();
            break;
    }
}
