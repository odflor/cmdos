#include <cbm.h>
#include <conio.h>
#include <string.h>
#include "cmdosk-internal.h"
#include "reu.h"

#define PICK_LFN 5
#define PICK_ROWS 9

static unsigned char read_page(unsigned int skip, char names[][17])
{
    struct cbm_dirent entry;
    unsigned char count = 0, result;

    if (!KRNL_REQUEST.device) {
        for (result = 0; result < reu_slots; ++result) {
            krnl_reu_dir_read(result);
            if (!*reu_dir_name || reu_dir_type != KRNL_REQUEST.style) continue;
            if (skip) --skip;
            else {
                if (count == PICK_ROWS) return count + 1;
                strcpy(names[count++], reu_dir_name);
            }
        }
        return count;
    }
    if (cbm_opendir(PICK_LFN, KRNL_REQUEST.device) != 0) {
        cbm_k_close(PICK_LFN);
        return 255;
    }
    while ((result = cbm_readdir(PICK_LFN, &entry)) == 0) {
        if (entry.type != KRNL_REQUEST.style) {
            continue;
        }
        if (skip) {
            --skip;
        } else {
            if (count == PICK_ROWS) {
                ++count; /* One extra match means another page exists. */
                break;
            }
            entry.name[16] = 0;
            strcpy(names[count++], entry.name);
        }
    }
    cbm_closedir(PICK_LFN);
    return result == 0 || result == 2 ? count : 255;
}

void krnl_tui_pick_file(void)
{
    char names[PICK_ROWS][17];
    unsigned int skip = 0;
    unsigned char count, row, selected = 0, key, loaded = 0;
    const unsigned char* text;

    KRNL_REQUEST.result = 0;
    krnl_tui_box(21, 4, 38, 14, KRNL_TUI_POPUP);
    krnl_tui_text_span(22, 5, 36, KRNL_TUI_POPUP | 0x80, (const unsigned char*)" Seleccionar archivo");
    krnl_tui_text_span(22, 16, 36, KRNL_TUI_POPUP, (const unsigned char*)" Flechas/Enter  Esc");
    for (;;) {
        if (!loaded) {
            count = read_page(skip, names);
            if (count && count < PICK_ROWS && selected >= count) {
                selected = count - 1;
            }
            loaded = 1;
        }
        for (row = 0; row < PICK_ROWS; ++row) {
            text = (const unsigned char*)"";
            if (count == 255) {
                if (row == 0) text = (const unsigned char*)" Error de lectura";
            } else if (row < count) {
                text = (const unsigned char*)names[row];
            } else if (count == 0 && row == 0) {
                text = (const unsigned char*)" Sin archivos";
            }
            krnl_tui_text_span(22, 6 + row, 36, KRNL_TUI_POPUP | ((count && count != 255 && row == selected) ? 0x80 : 0), text);
        }
        key = cgetc();
        if (key == 27) {
            return;
        }
        if (count == 0 || count == 255) {
            continue;
        }
        if (key == CH_ENTER || key == 10) {
            strcpy((char*)KRNL_REQUEST.data, names[selected]);
            KRNL_REQUEST.result = KRNL_PICK_SELECTED;
            return;
        }
        if (key == CH_CURS_DOWN && selected + 1 < count) {
            if (++selected == PICK_ROWS) {
                skip += PICK_ROWS;
                selected = loaded = 0;
            }
        } else if (key == CH_CURS_UP) {
            if (selected) {
                --selected;
            } else if (skip) {
                skip -= PICK_ROWS;
                selected = PICK_ROWS - 1;
                loaded = 0;
            }
        }
    }
}
