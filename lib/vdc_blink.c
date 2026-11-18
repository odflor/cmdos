#include <conio.h>
#include <c128.h>

/* VDC I/O ports: $D600 selects a register and reads status; $D601 is data. */
#define VDC_PORT       ((volatile unsigned char*)0xD600)
#define VDC_REG_STATUS VDC_PORT[0]
#define VDC_DATA       VDC_PORT[1]

#define VDC_READY      0x80
#define VDC_ATTR_BASE  0x0800
#define SCREEN_WIDTH   80
#define VDC_LIGHTBLUE  0x03  /* VDC RGBI order: blue + intensity */

/* VDC character attribute flags; combine them with the bitwise OR operator. */
enum VDCTextEffect {
    VDC_EFFECT_NONE      = 0x00,
    VDC_EFFECT_BLINK     = 0x10,
    VDC_EFFECT_UNDERLINE = 0x20
};

static void vdc_wait_ready(void)
{
    while ((VDC_REG_STATUS & VDC_READY) == 0) {
        /* The VDC is still busy with its previous operation. */
    }
}

static void vdc_set_update_address(unsigned int address)
{
    vdc_wait_ready();
    VDC_REG_STATUS = 18;               /* Update address, high byte */
    vdc_wait_ready();
    VDC_DATA = (unsigned char)(address >> 8);

    vdc_wait_ready();
    VDC_REG_STATUS = 19;               /* Update address, low byte */
    vdc_wait_ready();
    VDC_DATA = (unsigned char)address;
}

static void vdc_apply_text_effects(unsigned char x, unsigned char y,
                                   unsigned char length,
                                   unsigned char effects)
{
    unsigned int address;
    unsigned char i;
    unsigned char attributes[SCREEN_WIDTH];

    if (length == 0 || length > SCREEN_WIDTH || x + length > SCREEN_WIDTH) {
        return;
    }

    /* Default C128 80-column attribute memory begins at $0800. */
    address = VDC_ATTR_BASE + (unsigned int)y * SCREEN_WIDTH + x;
    vdc_set_update_address(address);

    /* Read the attributes created by cc65, preserving charset and colors. */
    vdc_wait_ready();
    VDC_REG_STATUS = 31;
    for (i = 0; i < length; ++i) {
        vdc_wait_ready();
        attributes[i] = VDC_DATA;
    }

    /* Rewind and add the requested flags without changing other attributes. */
    vdc_set_update_address(address);
    vdc_wait_ready();
    VDC_REG_STATUS = 31;
    for (i = 0; i < length; ++i) {
        vdc_wait_ready();
        VDC_DATA = attributes[i] | effects;
    }
}

static void vdc_put_screen_cell(unsigned char x, unsigned char y,
                                unsigned char screen_code)
{
    unsigned int address;
    unsigned char attribute;

    address = (unsigned int)y * SCREEN_WIDTH + x;
    vdc_set_update_address(address);
    vdc_wait_ready();
    VDC_REG_STATUS = 31;
    vdc_wait_ready();
    VDC_DATA = screen_code;

    address = VDC_ATTR_BASE + (unsigned int)y * SCREEN_WIDTH + x;
    vdc_set_update_address(address);
    vdc_wait_ready();
    VDC_REG_STATUS = 31;
    vdc_wait_ready();
    attribute = VDC_DATA;

    /* Use uppercase/graphics charset and bright blue, preserving other flags. */
    vdc_set_update_address(address);
    vdc_wait_ready();
    VDC_REG_STATUS = 31;
    vdc_wait_ready();
    VDC_DATA = (attribute & 0x70) | VDC_LIGHTBLUE;
}

static void draw_box_shadow(void)
{
    unsigned char y;
    unsigned char x;

    /* A one-cell offset in gray gives the frame a simple drop shadow. */
    textcolor(COLOR_GRAY2);
    revers(1);
    for (y = 9; y <= 18; ++y) {
        gotoxy(50, y);
        cputc(' ');
    }
    for (x = 9; x <= 50; ++x) {
        gotoxy(x, 18);
        cputc(' ');
    }
    revers(0);
}

static void draw_rounded_box(void)
{
    unsigned char x;
    unsigned char y;

    /* Raw VDC screen codes for square box corners. */
    vdc_put_screen_cell(8, 8, 0x70);   /* Upper-left */
    vdc_put_screen_cell(49, 8, 0x6E);  /* Upper-right */
    vdc_put_screen_cell(8, 17, 0x6D);  /* Lower-left */
    vdc_put_screen_cell(49, 17, 0x7D); /* Lower-right */

    for (x = 9; x < 49; ++x) {
        vdc_put_screen_cell(x, 8, 0x40);  /* Horizontal line */
        vdc_put_screen_cell(x, 17, 0x40);
    }
    for (y = 9; y < 17; ++y) {
        vdc_put_screen_cell(8, y, 0x5D);  /* Vertical line */
        vdc_put_screen_cell(49, y, 0x5D);
    }
}

void main(void)
{
    videomode(VIDEOMODE_80x25);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    revers(0);
    clrscr();

    draw_box_shadow();
    draw_rounded_box();

    textcolor(COLOR_CYAN);
    gotoxy(13, 9);
    cputs("=== VDC 80-COLUMN BLINK TEST ===");

    /* Divider directly below the title, spanning the inside of the window. */
    {
        unsigned char x;
        for (x = 9; x < 49; ++x) {
            vdc_put_screen_cell(x, 10, 0x40);
        }
    }
    /* Tee junctions connect the divider to both vertical sides. */
    vdc_put_screen_cell(8, 10, 0x6B);
    vdc_put_screen_cell(49, 10, 0x73);

    textcolor(COLOR_LIGHTGREEN);
    gotoxy(10, 11);
    cputs("THIS TEXT BLINKS ON THE C128 VDC!");
    vdc_apply_text_effects(10, 11, 33, VDC_EFFECT_BLINK);

    textcolor(COLOR_YELLOW);
    gotoxy(10, 13);
    cputs("THIS TEXT IS UNDERLINED");
    vdc_apply_text_effects(10, 13, 23, VDC_EFFECT_UNDERLINE);

    textcolor(COLOR_LIGHTRED);
    gotoxy(10, 15);
    cputs("BLINKING AND UNDERLINED");
    vdc_apply_text_effects(10, 15, 23,
                           VDC_EFFECT_BLINK | VDC_EFFECT_UNDERLINE);

    textcolor(COLOR_WHITE);
    gotoxy(10, 20);
    cputs("Press a key to exit...");
    cgetc();
}
