#include <conio.h>
#include <string.h>
#include "cmdosk-context.h"
#include "cmdosk-internal.h"
#include "cmdosk-swap.h"
#include "reu.h"

#define SHIFT_FLAGS (*(volatile unsigned char*)0xD3)
#define C128_ALT 0x08
#define C128_TAB 0x09
#define CTRL_Y 0x19
#define MENU_WIDTH 30
#define MENU_X ((TUI_WIDTH - MENU_WIDTH) / 2)
#define SESSION_CANCEL 254
#define SESSION_SHELL 255

static const char digits[] = "0123456789abcdef";
static unsigned char pending_target;

unsigned char cmdosk_session_hotkey(unsigned char key)
{
    return key == CTRL_Y || (key == C128_TAB && (SHIFT_FLAGS & C128_ALT));
}

static void swap_message(const char* message)
{
    tui_box(MENU_X, 10, MENU_WIDTH, 3, TUI_POPUP);
    tui_text_span(MENU_X + 1, 11, MENU_WIDTH - 2, TUI_POPUP,
                  (const unsigned char*)message);
}

static void swap_error(const char* reason)
{
    const unsigned char x = 15, width = 50;

    tui_box(x, 9, width, 5, TUI_POPUP);
    tui_text_span(x + 1, 10, width - 2, TUI_POPUP | 0x80,
                  (const unsigned char*)" No se pudo suspender la sesion");
    tui_text_span(x + 1, 11, width - 2, TUI_POPUP,
                  (const unsigned char*)reason);
    tui_text_span(x + 1, 12, width - 2, TUI_POPUP,
                  (const unsigned char*)" Pulse una tecla para volver");
    cgetc();
}

static unsigned char choose_session(unsigned char from_app)
{
    unsigned char targets[17];
    unsigned char slot, count = 0;
    char* label = (char*)TUI_REQUEST.data + 3;

    if (from_app) {
        targets[count++] = SESSION_SHELL;
        memcpy(label, "Interprete", 11);
        label += 11;
    }
    for (slot = 0; slot < 16; ++slot) {
        reu_dir_read(slot);
        if (!*reu_dir_name || reu_dir_type != CMDOSK_SWAP_TYPE) continue;
        targets[count++] = slot;
        memcpy(label, "EDIT 0", 7);
        label[5] = reu_dir_name[6];
        label += 7;
    }
    if (!count) {
        cputs("Sin sesiones.\r\n");
        return SESSION_CANCEL;
    }

    TUI_REQUEST.data[0] = 1;
    TUI_REQUEST.data[1] = MENU_X;
    TUI_REQUEST.data[2] = count;
    TUI_REQUEST.count = (unsigned char)(label - (char*)TUI_REQUEST.data);
    TUI_REQUEST.y = (24 - count - 2) / 2;
    TUI_REQUEST.style = MENU_WIDTH;
    TUI_REQUEST.result = 0;
    cmdosk_menu();
    if (!TUI_REQUEST.result || TUI_REQUEST.result > count) return SESSION_CANCEL;
    return targets[TUI_REQUEST.result - 1];
}

void cmdosk_session_suspend(void)
{
    char name[] = "sesion0.swp";
    unsigned char i, target;

    if (!mounted) {
        swap_error(reu_detected == 2 ? " M: sin formato. Use FORMAT M:."
                                     : " M: requiere una REU de 16 MiB.");
        return;
    }
    if (mounted && !cmdosk_disk_busy()) {
        target = choose_session(1);
        if (target == SESSION_CANCEL) return;
        for (i = 0; i < 16; ++i) {
            name[6] = digits[i];
            if (!reu_find(name)) {
                swap_message("Guardando sesion en REU...");
                bank1_capture_context();
                cmdosk_context[CMDOSK_CONTEXT_DEVICE] = TUI_REQUEST.device;
                if (!cmdosk_swap_save(name, cmdosk_context, CMDOSK_CONTEXT_SIZE)) {
                    pending_target = target;
                    bank1_suspend_pending = 1;
                    return;
                }
                break;
            }
        }
    }
    swap_error(" Compruebe los archivos y el espacio de M:.");
}

static void resume_session(unsigned char slot)
{
    unsigned char device;
    char name[17];

    if (slot >= 16) return;
    reu_dir_read(slot);
    if (!*reu_dir_name || reu_dir_type != CMDOSK_SWAP_TYPE) return;
    strcpy(name, reu_dir_name);
    swap_message("Cargando sesion de REU...");

    bank1_prepare(8);
    if (cmdosk_swap_restore(name, cmdosk_context, CMDOSK_CONTEXT_SIZE) ||
        cmdosk_context[1] != 0x4e) {
        bank1_finish();
        tui_reset();
        cputs("Sesion invalida o incompatible.\r\n");
        return;
    }
    device = cmdosk_context[CMDOSK_CONTEXT_DEVICE];
    if (device && (device < 8 || device > 11)) {
        bank1_finish();
        tui_reset();
        cputs("Sesion invalida o incompatible.\r\n");
        return;
    }
    /* The running session has no directory entry until suspended again. */
    *reu_dir_name = 0;
    reu_buffer = reu_dir_name;
    reu_length = 1;
    reu_write_entry(slot);
    TUI_REQUEST.device = device;
    TUI_REQUEST.result = TUI_SESSION_RESUMED;
    tui_editor_keys(1);
    bank1_resume();
    tui_editor_keys(0);
    bank1_finish();
    tui_reset();
}

void cmdosk_session_menu(void)
{
    if (mounted) resume_session(choose_session(0));
    else cputs("Sesiones: M: sin montar.\r\n");
}

void cmdosk_session_switch_pending(void)
{
    resume_session(pending_target);
}
