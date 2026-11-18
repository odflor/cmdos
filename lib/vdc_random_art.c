#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <c128.h>

#define VDC_ADDR       (*(volatile unsigned char*)0xD600)
#define VDC_DATA       (*(volatile unsigned char*)0xD601)

#define SCREEN_WIDTH   80
#define SCREEN_HEIGHT  25
#define TOTAL_CELLS    (SCREEN_WIDTH * SCREEN_HEIGHT)
#define SCREEN_BASE    0x0000
#define ATTR_BASE      0x0800

#define VDC_READY      0x80

#define ATTR_BLINK     0x10
#define ATTR_UNDERLINE 0x20
#define ATTR_REVERSE   0x40
#define ATTR_ALTSET    0x80

/* $D600 reports VDC status when read; wait until its Update Ready bit is set. */
static void vdc_wait_ready(void)
{
    while ((VDC_ADDR & VDC_READY) == 0) {
    }
}

/* Set the VDC's 16-bit VRAM update pointer using registers R18 and R19. */
static void vdc_set_addr(unsigned int address)
{
    vdc_wait_ready();
    VDC_ADDR = 18;
    vdc_wait_ready();
    VDC_DATA = (unsigned char)(address >> 8);

    vdc_wait_ready();
    VDC_ADDR = 19;
    vdc_wait_ready();
    VDC_DATA = (unsigned char)address;
}

static void vdc_select_data_register(void)
{
    vdc_wait_ready();
    VDC_ADDR = 31;                 /* R31 accesses VRAM through $D601. */
}

/* Fill the 80 x 25 character screen with random screen codes. */
static void vdc_fill_chars(void)
{
    unsigned int i;

    vdc_set_addr(SCREEN_BASE);
    vdc_select_data_register();

    for (i = 0; i < TOTAL_CELLS; ++i) {
        /* Screen codes 32..127 contain a varied mix of letters and graphics. */
        unsigned char screen_code = (unsigned char)((rand() % 96) + 32);
        vdc_wait_ready();
        VDC_DATA = screen_code;
    }
}

/* Fill the matching attribute area with random colors and VDC effects. */
static void vdc_randomize_attributes(void)
{
    unsigned int i;

    vdc_set_addr(ATTR_BASE);
    vdc_select_data_register();

    for (i = 0; i < TOTAL_CELLS; ++i) {
        unsigned char attr = (unsigned char)(rand() % 16); /* RGBI color */

        if ((rand() % 4) == 0) {
            attr |= ATTR_REVERSE;       /* 25% reverse video */
        }
        if ((rand() % 6) == 0) {
            attr |= ATTR_BLINK;         /* About 17% blinking */
        }
        if ((rand() % 8) == 0) {
            attr |= ATTR_UNDERLINE;     /* 12.5% underlined */
        }
        if ((rand() % 2) == 0) {
            attr |= ATTR_ALTSET;        /* Mix the alternate character set */
        }

        vdc_wait_ready();
        VDC_DATA = attr;
    }
}

void main(void)
{
    videomode(VIDEOMODE_80x25);

    /* Seed from the C128 jiffy clock so each launch can produce new art. */
    srand((unsigned int)clock());

    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    revers(0);
    clrscr();

    vdc_fill_chars();
    vdc_randomize_attributes();

    while (!kbhit()) {
    }
    cgetc();
    clrscr();
}
