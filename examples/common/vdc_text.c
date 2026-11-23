#include <c128.h>
#include "vdc_text.h"

/* Fuente compacta 5x7: la rutina sigue la idea de OUTTEXT de c128-hi.tgi,
** pero escribe cada bit en la VRAM del VDC en lugar de la RAM del VIC. */
static const unsigned char font5x7[26][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {1,1,1,1,17,17,14},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};

static unsigned char scale_x = 1;
static unsigned char scale_y = 1;
static unsigned char direction = 0;

static unsigned char vdc_read(unsigned int address)
{
    VDC.ctrl = 18;
    VDC.data = (unsigned char)(address >> 8);
    VDC.ctrl = 19;
    VDC.data = (unsigned char)address;
    VDC.ctrl = 31;
    return VDC.data;
}

static void vdc_write(unsigned int address, unsigned char value)
{
    VDC.ctrl = 18;
    VDC.data = (unsigned char)(address >> 8);
    VDC.ctrl = 19;
    VDC.data = (unsigned char)address;
    VDC.ctrl = 31;
    VDC.data = value;
}

static void vdc_set_pixel(int x, int y)
{
    unsigned int address;
    unsigned char mask;
    unsigned char value;

    if (x < 0 || x >= 640 || y < 0 || y >= 200) return;
    address = (unsigned int)y * 80u + (unsigned int)(x >> 3);
    mask = (unsigned char)(0x80u >> (x & 7));
    value = vdc_read(address);
    vdc_write(address, (unsigned char)(value | mask));
}

void vdc_text_style(unsigned char magx, unsigned char magy, unsigned char dir)
{
    scale_x = magx ? magx : 1;
    scale_y = magy ? magy : 1;
    direction = dir;
}

void vdc_text_outtextxy(int x, int y, const char* text)
{
    unsigned char c, row, bit, sx, sy;
    int px, py;

    while (*text) {
        c = (unsigned char)*text++;
        if (c >= 'A' && c <= 'Z') {
            for (row = 0; row < 7; ++row) {
                for (bit = 0; bit < 5; ++bit) {
                    if (font5x7[c - 'A'][row] & (1u << (4 - bit))) {
                        for (sy = 0; sy < scale_y; ++sy) {
                            for (sx = 0; sx < scale_x; ++sx) {
                                px = x + bit * scale_x + sx;
                                py = y + row * scale_y + sy;
                                if (direction == 1) {
                                    px = x - row * scale_y - sy;
                                    py = y + bit * scale_x + sx;
                                }
                                vdc_set_pixel(px, py);
                            }
                        }
                    }
                }
            }
        }
        if (direction == 1) y += 6 * scale_x;
        else x += 6 * scale_x;
    }
}
