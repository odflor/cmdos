#include <cbm.h>
#include <conio.h>
#include <string.h>
#include "tui.h"

#define FILE_LFN 4
#define STATUS_LFN 15
#define MMU_RAM (*(volatile unsigned char*)0xD506)
#define PICK_LFN 5
#define PICK_ROWS 9

extern unsigned char bank1_gate[], bank1_gate_size[];
static unsigned char saved_ram, file_open;
static unsigned char saved_sprites[0x1F7];
static unsigned char saved_keys[18];

void tui_editor_keys(unsigned char enable)
{
    unsigned char* const keys = (unsigned char*)0x1000;
    static const unsigned char raw_keys[] = {
        CH_F1, CH_F2, CH_F3, CH_F4, CH_F5, CH_F6, CH_F7, CH_F8
    };

    if (enable) {
        memcpy(saved_keys, keys, sizeof(saved_keys));
        memset(keys, 1, sizeof(raw_keys));
        keys[8] = keys[9] = 0;
        memcpy(keys + 10, raw_keys, sizeof(raw_keys));
    } else {
        memcpy(keys, saved_keys, sizeof(saved_keys));
    }
}

void tui_reset(void)
{
    revers(0);
    cursor(1);
    bgcolor(TUI_BACKGROUND);
    textcolor(TUI_TEXT);
    clrscr();
}

void bank1_prepare(unsigned char device)
{
    unsigned int i;
    unsigned char* const common = (unsigned char*)0x0E00;

    for (i = 0; i < sizeof(saved_sprites); ++i) {
        saved_sprites[i] = common[i];
    }
    for (i = 0; i < (unsigned int)bank1_gate_size; ++i) {
        common[i] = bank1_gate[i];
    }
    saved_ram = MMU_RAM;
    MMU_RAM = (saved_ram & 0xF0) | 0x05; /* Bottom 4 KiB common. */
    TUI_REQUEST.device = device;
    file_open = 0;
}

void bank1_finish(void)
{
    unsigned int i;
    unsigned char* const common = (unsigned char*)0x0E00;

    if (file_open) {
        cbm_k_close(FILE_LFN);
        cbm_k_close(STATUS_LFN);
    }
    MMU_RAM = saved_ram;
    for (i = 0; i < sizeof(saved_sprites); ++i) {
        common[i] = saved_sprites[i];
    }
}

static unsigned char disk_status(void)
{
    unsigned char first, second, status = 255;

    if (cbm_k_chkin(STATUS_LFN) == 0) {
        first = cbm_k_basin() - '0';
        second = cbm_k_basin() - '0';
        if (first <= 9 && second <= 9) {
            status = first * 10 + second;
        }
        while (!cbm_k_readst() && cbm_k_basin() != CH_ENTER) {
        }
    }
    cbm_k_clrch();
    return status;
}

static void text_span(unsigned char x, unsigned char y, unsigned char width,
                      unsigned char style, const unsigned char* text)
{
    gotoxy(x, y);
    textcolor(style & 15);
    revers(style >> 7);
    while (width--) {
        cputc(*text ? *text++ : ' ');
    }
    revers(0);
}

static void box(unsigned char x, unsigned char y, unsigned char width,
                unsigned char height, unsigned char style)
{
    unsigned char row;

    cursor(0);
    textcolor(style & 15);
    revers(style >> 7);
    for (row = 0; row < height; ++row) {
        gotoxy(x, y + row);
        if (row == 0 || row == height - 1) {
            cputc(row == 0 ? CH_ULCORNER : CH_LLCORNER);
            chline(width - 2);
            cputc(row == 0 ? CH_URCORNER : CH_LRCORNER);
        } else {
            cputc(CH_VLINE);
            cclear(width - 2);
            cputc(CH_VLINE);
        }
    }
    revers(0);
}

static unsigned char read_page(unsigned int skip, char names[][17])
{
    struct cbm_dirent entry;
    unsigned char count = 0, result;

    if (cbm_opendir(PICK_LFN, TUI_REQUEST.device) != 0) {
        cbm_k_close(PICK_LFN);
        return 255;
    }
    while ((result = cbm_readdir(PICK_LFN, &entry)) == 0) {
        if (entry.type != TUI_REQUEST.style) {
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

static void pick_file(void)
{
    char names[PICK_ROWS][17];
    unsigned int skip = 0;
    unsigned char count, row, selected = 0, key, loaded = 0;
    const unsigned char* text;

    TUI_REQUEST.result = 0;
    box(21, 4, 38, 14, TUI_POPUP);
    text_span(22, 5, 36, TUI_POPUP | 0x80, (const unsigned char*)" Seleccionar archivo");
    text_span(22, 16, 36, TUI_POPUP, (const unsigned char*)" Flechas / Enter     Esc cancela");
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
                if (row == 0) text = (const unsigned char*)" No se pudo leer la unidad";
            } else if (row < count) {
                text = (const unsigned char*)names[row];
            } else if (count == 0 && row == 0) {
                text = (const unsigned char*)" No hay archivos de ese tipo";
            }
            text_span(22, 6 + row, 36, TUI_POPUP | ((count && count != 255 && row == selected) ? 0x80 : 0), text);
        }
        key = cgetc();
        if (key == 27) {
            return;
        }
        if (count == 0 || count == 255) {
            continue;
        }
        if (key == CH_ENTER || key == 10) {
            strcpy((char*)TUI_REQUEST.data, names[selected]);
            TUI_REQUEST.result = 1;
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

void tui_dispatch(void)
{
    int count;

    TUI_REQUEST.result = 0;
    switch (TUI_REQUEST.operation) {
        case TUI_CLEAR:
            tui_reset();
            cursor(0);
            break;
        case TUI_DRAW:
            if (TUI_REQUEST.y < 24 && TUI_REQUEST.x < TUI_WIDTH &&
                TUI_REQUEST.count <= TUI_WIDTH - TUI_REQUEST.x) {
                text_span(TUI_REQUEST.x, TUI_REQUEST.y, TUI_REQUEST.count,
                          TUI_REQUEST.style, TUI_REQUEST.data);
            }
            break;
        case TUI_DIALOG:
            box(15, 8, 50, 5, TUI_POPUP | 0x80);
            text_span(16, 9, 48, TUI_POPUP | 0x80, TUI_REQUEST.data);
            text_span(16, 10, 48, TUI_POPUP | 0x80, TUI_REQUEST.data + 50);
            break;
        case TUI_BOX:
            if (TUI_REQUEST.count >= 2 && TUI_REQUEST.data[0] >= 2 &&
                TUI_REQUEST.x + TUI_REQUEST.count <= TUI_WIDTH &&
                TUI_REQUEST.y + TUI_REQUEST.data[0] <= 24) {
                box(TUI_REQUEST.x, TUI_REQUEST.y, TUI_REQUEST.count,
                    TUI_REQUEST.data[0], TUI_REQUEST.style);
            }
            break;
        case TUI_DISK_COMMAND:
            TUI_REQUEST.result = 255;
            if (!file_open && cbm_open(STATUS_LFN, TUI_REQUEST.device, 15,
                                       (char*)TUI_REQUEST.data) == 0) {
                TUI_REQUEST.result = disk_status();
                cbm_k_close(STATUS_LFN);
            }
            break;
        case TUI_PICK_FILE:
            if (!file_open) {
                pick_file();
            }
            break;
        case TUI_KEY:
            TUI_REQUEST.result = cgetc();
            break;
        case TUI_CURSOR:
            gotoxy(TUI_REQUEST.x, TUI_REQUEST.y);
            cursor(1);
            break;
        case TUI_READ_OPEN:
        case TUI_WRITE_OPEN:
            if (file_open || cbm_open(STATUS_LFN, TUI_REQUEST.device, 15, "") != 0) {
                TUI_REQUEST.result = 1;
                break;
            }
            if (cbm_open(FILE_LFN, TUI_REQUEST.device, CBM_SEQ,
                         (char*)TUI_REQUEST.data) != 0 || disk_status()) {
                cbm_k_close(FILE_LFN);
                cbm_k_close(STATUS_LFN);
                TUI_REQUEST.result = 1;
                break;
            }
            file_open = 1;
            break;
        case TUI_READ:
            if (!file_open || TUI_REQUEST.count > TUI_TRANSFER) {
                TUI_REQUEST.result = 255;
                break;
            }
            count = cbm_read(FILE_LFN, TUI_REQUEST.data, TUI_REQUEST.count);
            TUI_REQUEST.result = count < 0 ? 255 : (unsigned char)count;
            break;
        case TUI_WRITE:
            if (!file_open || TUI_REQUEST.count > TUI_TRANSFER) {
                TUI_REQUEST.result = 1;
                break;
            }
            count = cbm_write(FILE_LFN, TUI_REQUEST.data, TUI_REQUEST.count);
            TUI_REQUEST.result = count != TUI_REQUEST.count;
            break;
        case TUI_CLOSE:
            cbm_k_close(FILE_LFN);
            TUI_REQUEST.result = disk_status();
            cbm_k_close(STATUS_LFN);
            file_open = 0;
            break;
    }
}
