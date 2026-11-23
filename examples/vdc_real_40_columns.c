#include <conio.h>
#include <c128.h>

#define VDC_REGISTER       (*(volatile unsigned char*)0xD600)
#define VDC_DATA           (*(volatile unsigned char*)0xD601)
#define VDC_READY          0x80
#define SCREEN_WIDTH       40

/* Active C128 screen-editor variables: left/right margin and max columns. */
#define SCREEN_LEFT_MARGIN (*(volatile unsigned char*)0x00E6)
#define SCREEN_RIGHT_MARGIN (*(volatile unsigned char*)0x00E7)
#define SCREEN_MAX_COLUMNS (*(volatile unsigned char*)0x00EE)

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

static void vdc_set_40_columns(void)
{
    vdc_write_register(0, 63);       /* Half-width horizontal total */
    vdc_write_register(1, 40);       /* 40 visible character cells */
    vdc_write_register(2, 55);       /* Center the VDC image */
    vdc_write_register(22, 137);     /* Double-width character pixels */
    vdc_write_register(25, vdc_read_register(25) | 0x10);
    /* R27 stays at 40: 40 visible + 40 hidden = 80-byte KERNAL row stride. */
    vdc_write_register(27, 40);
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
    videomode(VIDEOMODE_80x25);      /* Keep the KERNAL writing to the VDC. */
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    revers(0);
    clrscr();

    vdc_set_40_columns();

    /* Restrict the active BASIC/KERNAL editor window to columns 0 through 39. */
    SCREEN_LEFT_MARGIN = 0;
    SCREEN_RIGHT_MARGIN = SCREEN_WIDTH - 1;
    SCREEN_MAX_COLUMNS = SCREEN_WIDTH - 1;
    clrscr();

    put_centered_text(1, COLOR_CYAN, 1,
                      "VDC REAL: 40 COLUMNAS EN BASIC");

    put_centered_text(3, COLOR_WHITE, 0,
                      "Lorem ipsum dolor sit amet,");
    put_text(2, 4, COLOR_YELLOW, 0, "consectetur ");
    put_text(14, 4, COLOR_WHITE, 0, "adipiscing elit. Sed do");
    put_text(2, 5, COLOR_LIGHTGREEN, 0, "eiusmod ");
    put_text(10, 5, COLOR_WHITE, 0, "tempor incididunt ut labore");
    put_centered_text(6, COLOR_WHITE, 0,
                      "et dolore magna aliqua. Ut enim ad");
    put_centered_text(7, COLOR_WHITE, 0,
                      "MINIM VENIAM, quis nostrud");
    put_centered_text(8, COLOR_WHITE, 0,
                      "exercitation ullamco laboris nisi ut");
    put_text(4, 9, COLOR_WHITE, 0, "aliquip ex ea ");
    put_text(18, 9, COLOR_LIGHTRED, 0, "commodo ");
    put_text(26, 9, COLOR_WHITE, 0, "consequat.");

    put_centered_text(11, COLOR_WHITE, 0,
                      "Duis aute irure dolor in");
    put_text(3, 12, COLOR_LIGHTBLUE, 0, "REPREHENDERIT ");
    put_text(17, 12, COLOR_WHITE, 0, "in voluptate velit");
    put_centered_text(13, COLOR_WHITE, 0,
                      "esse cillum dolore eu fugiat nulla");
    put_text(3, 14, COLOR_GREEN, 0, "pariatur");
    put_text(11, 14, COLOR_WHITE, 0,
             ". Excepteur sint occaecat");
    put_centered_text(15, COLOR_WHITE, 0,
                      "CUPIDATAT non proident, sunt in");
    put_centered_text(16, COLOR_WHITE, 0,
                      "culpa qui officia deserunt mollit");
    put_centered_text(17, COLOR_WHITE, 0,
                      "anim id est laborum.");

    put_centered_text(19, COLOR_GRAY3, 0,
                      "Editor BASIC limitado a 40 columnas");
    put_text(1, 21, COLOR_WHITE, 0,
             "Escriba en BASIC: al llegar a la columna 40");
    put_text(1, 22, COLOR_LIGHTBLUE, 0,
             "el cursor debe continuar en la linea siguiente.");

    /* Leave the screen and editor configuration active for BASIC. */
    revers(0);
    textcolor(COLOR_WHITE);
    cursor(1);
    gotoxy(0, 24);
}
