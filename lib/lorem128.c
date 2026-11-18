#include <conio.h>
#include <c128.h>

#define VDC_REGISTER   (*(volatile unsigned char*)0xD600)
#define VDC_DATA       (*(volatile unsigned char*)0xD601)
#define VDC_READY      0x80
#define SCREEN_WIDTH   40

/* Wait for the VDC before selecting or writing one of its registers. */
static void vdc_wait_ready(void)
{
    while ((VDC_REGISTER & VDC_READY) == 0) {
    }
}

static void vdc_write_register(unsigned char reg, unsigned char value)
{
    vdc_wait_ready();
    VDC_REGISTER = reg;
    vdc_wait_ready();
    VDC_DATA = value;
}

static unsigned char vdc_read_register(unsigned char reg)
{
    vdc_wait_ready();
    VDC_REGISTER = reg;
    vdc_wait_ready();
    return VDC_DATA;
}

/*
** Keep the KERNAL/conio screen in 80-column mode, but display 40 doubled
** pixels per row. Keep the KERNAL's vertical PAL/NTSC timing, screen and font
** addresses, attribute setup, and VDC revision bits. Pixel doubling requires
** halving the horizontal total; R2 is five positions below the 40-column
** reference value to move the displayed image right. R27 skips the remaining
** 40 cells in each 80-cell KERNAL row.
*/
static void vdc_set_40_columns(void)
{
    vdc_write_register(0, 63);        /* Half-width horizontal total */
    vdc_write_register(1, 40);        /* Horizontal displayed characters */
    vdc_write_register(2, 55);        /* Reference 40-column position */
    vdc_write_register(22, 137);      /* $89: double-pixel character width */
    vdc_write_register(25, vdc_read_register(25) | 0x10);
                                      /* Double pixels; preserve other bits */
    vdc_write_register(27, 40);       /* Keep 80-byte KERNAL row stride */
}

static unsigned char centered_x(const char* text)
{
    unsigned char length = 0;

    while (text[length] != '\0') {
        ++length;
    }
    return (SCREEN_WIDTH - length) / 2;
}

static void put_text(unsigned char x, unsigned char y, unsigned char color,
                     unsigned char reverse, const char* text)
{
    gotoxy(x, y);
    textcolor(color);
    revers(reverse);
    cputs(text);
    revers(0);
}

static void put_centered_text(unsigned char y, unsigned char color,
                              unsigned char reverse, const char* text)
{
    put_text(centered_x(text), y, color, reverse, text);
}

void main(void)
{
    videomode(VIDEOMODE_80x25);       /* Use the VDC/KERNAL console path. */
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    revers(0);
    clrscr();

    vdc_set_40_columns();
    clrscr();

    put_text(5, 1, COLOR_CYAN, 1, "DEMO VDC 40 COLUMNAS - C128");

    /* Center each manually wrapped line within the 40 displayed columns. */
    put_centered_text(3, COLOR_WHITE, 0, "Lorem ipsum dolor sit amet,");
    put_text(2, 4, COLOR_YELLOW, 0, "consectetur ");
    put_text(14, 4, COLOR_WHITE, 0, "adipiscing elit. Sed do");
    put_text(2, 5, COLOR_LIGHTGREEN, 0, "eiusmod ");
    put_text(10, 5, COLOR_WHITE, 0, "tempor incididunt ut labore");
    put_centered_text(6, COLOR_WHITE, 0, "et dolore magna aliqua. Ut enim ad");
    put_centered_text(7, COLOR_WHITE, 0, "MINIM VENIAM, quis nostrud");
    put_centered_text(8, COLOR_WHITE, 0, "exercitation ullamco laboris nisi ut");
    put_text(4, 9, COLOR_WHITE, 0, "aliquip ex ea ");
    put_text(18, 9, COLOR_LIGHTRED, 0, "commodo ");
    put_text(26, 9, COLOR_WHITE, 0, "consequat.");

    put_centered_text(11, COLOR_WHITE, 0, "Duis aute irure dolor in");
    put_text(3, 12, COLOR_LIGHTBLUE, 0, "REPREHENDERIT ");
    put_text(17, 12, COLOR_WHITE, 0, "in voluptate velit");
    put_centered_text(13, COLOR_WHITE, 0, "esse cillum dolore eu fugiat nulla");
    put_text(3, 14, COLOR_GREEN, 0, "pariatur");
    put_text(11, 14, COLOR_WHITE, 0, ". Excepteur sint occaecat");
    put_centered_text(15, COLOR_WHITE, 0, "CUPIDATAT non proident, sunt in");
    put_centered_text(16, COLOR_WHITE, 0, "culpa qui officia deserunt mollit");
    put_centered_text(17, COLOR_WHITE, 0, "anim id est laborum.");

    put_centered_text(19, COLOR_GRAY3, 0,
                      "Atributos VDC: color y video reverso");
    put_text(1, 21, COLOR_WHITE, 0, "Normal: blanco");
    put_text(17, 21, COLOR_WHITE, 0, "Reverso:");
    put_text(27, 21, COLOR_WHITE, 1, " CONTRASTE ");

    put_text(9, 20, COLOR_WHITE, 0, "Colores:");
    put_text(17, 20, COLOR_RED, 0, "R ");
    put_text(20, 20, COLOR_GREEN, 0, "G ");
    put_text(23, 20, COLOR_BLUE, 0, "B ");
    put_text(26, 20, COLOR_YELLOW, 0, "Y ");
    put_text(29, 20, COLOR_CYAN, 0, "C ");
    put_text(32, 20, COLOR_PURPLE, 0, "M ");
    put_text(35, 20, COLOR_WHITE, 0, "W");

    /* Group numbers aligned over each zero in the 80-character ruler. */
    put_text(0, 22, COLOR_WHITE, 0,
             "         1         2         3         4         5         6         7         8");

    /* 80-character ruler to check the visible width of the custom VDC mode. */
    put_text(0, 23, COLOR_YELLOW, 0,
             "12345678901234567890123456789012345678901234567890123456789012345678901234567890");

    put_centered_text(24, COLOR_LIGHTBLUE, 0,
                      "Presione una tecla para finalizar...");
    cgetc();
}
