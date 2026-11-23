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
#define MENU_X ((KRNL_TUI_WIDTH - MENU_WIDTH) / 2)
#define SESSION_DISCARD 253
#define SESSION_CANCEL 254
#define SESSION_SHELL 255

static const char digits[] = "0123456789abcdef";
static unsigned char pending_target;

unsigned char krnl_session_hotkey(unsigned char key)
{
    return key == CTRL_Y || (key == C128_TAB && (SHIFT_FLAGS & C128_ALT));
}

static void swap_message(const char* message)
{
    krnl_tui_box(MENU_X, 10, MENU_WIDTH, 3, KRNL_TUI_POPUP);
    krnl_tui_text_span(MENU_X + 1, 11, MENU_WIDTH - 2, KRNL_TUI_POPUP,
                  (const unsigned char*)message);
}

static void swap_error(const char* reason)
{
    const unsigned char x = 15, width = 50;

    krnl_tui_box(x, 9, width, 5, KRNL_TUI_POPUP);
    krnl_tui_text_span(x + 1, 10, width - 2, KRNL_TUI_POPUP | 0x80,
                  (const unsigned char*)" No se pudo cambiar de sesion");
    krnl_tui_text_span(x + 1, 11, width - 2, KRNL_TUI_POPUP,
                  (const unsigned char*)reason);
    krnl_tui_text_span(x + 1, 12, width - 2, KRNL_TUI_POPUP,
                  (const unsigned char*)" Pulse una tecla para volver");
    cgetc();
}

static unsigned char session_disk_busy(void)
{
    if (!krnl_disk_busy()) return 0;
    swap_error(" Cierre los canales de disco abiertos.");
    return 1;
}

static unsigned char session_entry(unsigned char slot)
{
    krnl_reu_dir_read(slot);
    return *reu_dir_name && reu_dir_type == KRNL_SWAP_TYPE;
}

static void remove_session(unsigned char slot)
{
    *reu_dir_name = 0;
    reu_buffer = reu_dir_name;
    reu_length = 1;
    krnl_reu_write_entry(slot);
}

static unsigned char choose_session(unsigned char from_app, unsigned char discard)
{
    unsigned char targets[18];
    unsigned char slot, count = 0;
    char* label = (char*)KRNL_REQUEST.data + 3;

    if (from_app) {
        targets[count++] = SESSION_SHELL;
        memcpy(label, "Interprete", 11);
        label += 11;
    }
    for (slot = 0; slot < 16; ++slot) {
        if (!session_entry(slot) ||
            (!discard && !krnl_swap_available(slot, KRNL_CONTEXT_SIZE))) continue;
        targets[count++] = slot;
        memcpy(label, "EDIT0", 6);
        label[4] = reu_dir_name[6];
        label += 6;
    }
    if (!discard) {
        targets[count++] = SESSION_DISCARD;
        memcpy(label, "Descartar", 10);
        label += 10;
    }
    if (!count) {
        cputs("Sin sesiones.\r\n");
        return SESSION_CANCEL;
    }

    KRNL_REQUEST.data[0] = 1;
    KRNL_REQUEST.data[1] = MENU_X;
    KRNL_REQUEST.data[2] = count;
    KRNL_REQUEST.count = (unsigned char)(label - (char*)KRNL_REQUEST.data);
    KRNL_REQUEST.y = (24 - count - 2) / 2;
    KRNL_REQUEST.style = MENU_WIDTH;
    KRNL_REQUEST.result = 0;
    krnl_tui_menu();
    if (!KRNL_REQUEST.result || KRNL_REQUEST.result > count) return SESSION_CANCEL;
    return targets[KRNL_REQUEST.result - 1];
}

static unsigned char confirm(void)
{
    unsigned char key;

    do {
        key = cgetc();
    } while (key != 's' && key != 'S' && key != 'n' && key != 'N' && key != CH_ESC);
    return key == 's' || key == 'S';
}

static unsigned char select_session(unsigned char from_app)
{
    unsigned char slot = choose_session(from_app, 0);

    if (slot != SESSION_DISCARD) return slot;
    slot = choose_session(0, 1);
    if (slot < 16 && session_entry(slot)) {
        swap_message("Descartar sesion? S/N/Esc");
        if (confirm()) remove_session(slot);
    }
    return SESSION_CANCEL;
}

void krnl_session_boot_notice(void)
{
    unsigned char slot;

    if (!mounted) return;
    for (slot = 0; slot < 16; ++slot) {
        if (session_entry(slot) && krnl_swap_available(slot, KRNL_CONTEXT_SIZE)) {
            cputs("Sesiones en M:. Use Alt+Tab.\r\n");
            return;
        }
    }
}

unsigned char krnl_session_confirm_exit(void)
{
    unsigned char slot, found = 0;

    if (!mounted) return 1;
    for (slot = 0; slot < 16; ++slot) {
        if (!session_entry(slot)) continue;
        if (!found) cputs("Sesiones suspendidas en M:\r\n");
        found = 1;
        cputs(reu_dir_name);
        cputs("\r\n");
    }
    if (!found) return 1;
    cputs("Se pierden si la REU no se conserva.\r\nApagar? S/N/Esc ");
    found = confirm();
    cputs("\r\n");
    return found;
}

void krnl_session_suspend(void)
{
    char name[] = "sesion0.swp";
    unsigned char i, target;

    if (!mounted) {
        swap_error(reu_detected == 2 ? " M: sin formato. Use FORMAT M:."
                                     : " M: requiere una REU de 16 MiB.");
        return;
    }
    if (session_disk_busy()) return;
    target = select_session(1);
    if (target == SESSION_CANCEL) return;
    for (i = 0; i < 16; ++i) {
        name[6] = digits[i];
        if (!krnl_reu_find(name)) {
            swap_message("Guardando sesion en REU...");
            krnl_bank1_capture_context();
            cmdosk_context[KRNL_CONTEXT_DEVICE] = KRNL_REQUEST.device;
            if (!krnl_swap_save(name, cmdosk_context, KRNL_CONTEXT_SIZE)) {
                pending_target = target;
                bank1_suspend_pending = 1;
                return;
            }
            break;
        }
    }
    swap_error(" Compruebe los archivos y el espacio de M:.");
}

static void resume_session(unsigned char slot)
{
    unsigned char device;
    char name[17];

    if (slot >= 16 || session_disk_busy()) return;
    krnl_reu_dir_read(slot);
    if (!*reu_dir_name || reu_dir_type != KRNL_SWAP_TYPE) return;
    strcpy(name, reu_dir_name);
    swap_message("Cargando sesion de REU...");

    krnl_bank1_prepare(8);
    if (krnl_swap_restore(name, cmdosk_context, KRNL_CONTEXT_SIZE) ||
        cmdosk_context[1] != 0x4e) {
        krnl_bank1_finish();
        krnl_tui_reset();
        cputs("Sesion invalida o incompatible.\r\n");
        return;
    }
    device = cmdosk_context[KRNL_CONTEXT_DEVICE];
    if (device && (device < 8 || device > 11)) {
        krnl_bank1_finish();
        krnl_tui_reset();
        cputs("Sesion invalida o incompatible.\r\n");
        return;
    }
    remove_session(slot);
    KRNL_REQUEST.device = device;
    KRNL_REQUEST.count = KRNL_SESSION_RESUMED;
    krnl_keyboard_editor_keys(1);
    krnl_bank1_resume();
    krnl_keyboard_editor_keys(0);
    krnl_bank1_finish();
    krnl_tui_reset();
}

void krnl_session_menu(void)
{
    if (session_disk_busy()) return;
    if (mounted) resume_session(select_session(0));
    else cputs("Sesiones: M: sin montar.\r\n");
}

void krnl_session_switch_pending(void)
{
    resume_session(pending_target);
}
