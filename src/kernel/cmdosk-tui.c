#include <conio.h>
#include "cmdosk-internal.h"

void krnl_tui_reset(void)
{
    revers(0);
    cursor(1);
    bgcolor(KRNL_TUI_BACKGROUND);
    textcolor(KRNL_TUI_TEXT);
    clrscr();
}

void krnl_tui_text_span(unsigned char x, unsigned char y, unsigned char width,
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

void krnl_tui_box(unsigned char x, unsigned char y, unsigned char width,
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
