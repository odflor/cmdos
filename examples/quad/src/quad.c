#include <c128.h>
#include <conio.h>
#include <stdio.h>
#include <tgi.h>

static const unsigned char palette[2] = {
    TGI_COLOR_BLACK, TGI_COLOR_WHITE
};

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

static const unsigned char digits5x7[10][7] = {
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
};

static const unsigned char glyph_dot[7] = {0,0,0,0,0,0,4};
static const unsigned char glyph_dash[7] = {0,0,0,31,0,0,0};
static const unsigned char glyph_underscore[7] = {0,0,0,0,0,0,31};
static const unsigned char glyph_slash[7] = {1,2,2,4,8,8,16};
static const unsigned char glyph_blank[7] = {0,0,0,0,0,0,0};

static const unsigned char* glyph_for(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return font5x7[c - 'A'];
    if (c >= '0' && c <= '9') return digits5x7[c - '0'];
    if (c == '.') return glyph_dot;
    if (c == '-' || c == 0x96) return glyph_dash;
    if (c == '_') return glyph_underscore;
    if (c == '/') return glyph_slash;
    return glyph_blank;
}

static unsigned char check_tgi(const char* step)
{
    unsigned char error = tgi_geterror();
    if (error != TGI_ERR_OK) {
        cprintf("tgi error en %s: %u\r\n", step, error);
        cputs("Pulsa una tecla para volver a BASIC.\r\n");
        cgetc();
    }
    return error;
}

static void draw_text_bitmap(int x, int y, const char* text, unsigned char scale)
{
    unsigned char c, row, bit, i;
    const unsigned char* glyph;
    tgi_setcolor(1);
    while (*text) {
        c = (unsigned char)*text++;
        glyph = glyph_for(c);
        for (row = 0; row < 7; ++row) {
            for (bit = 0; bit < 5; ++bit) {
                if (glyph[row] & (1 << (4 - bit))) {
                    for (i = 0; i < scale; ++i) {
                        tgi_line(x + bit * scale, y + row * scale + i,
                                 x + bit * scale + scale - 1, y + row * scale + i);
                    }
                }
            }
        }
        x += 6 * scale;
    }
}

static void abort_tgi(void)
{
    tgi_uninstall();
    videomode(VIDEOMODE_80COL);
    clrscr();
    cursor(1);
    cputs("quad: TGI descargado correctamente.\r\n");
}

static void diamond(int cx, int cy, int size)
{
    tgi_line(cx, cy - size, cx + size, cy);
    tgi_line(cx + size, cy, cx, cy + size);
    tgi_line(cx, cy + size, cx - size, cy);
    tgi_line(cx - size, cy, cx, cy - size);
}

static void draw_quad(void)
{
    const int left = 220;
    const int top = 20;
    const int right = 420;
    const int bottom = 180;
    int x;
    int y;

    tgi_clear();
    tgi_setcolor(1);

    /* Cuadrado centrado en el área 640x200 del VDC. */
    tgi_line(left, top, right, top);
    tgi_line(right, top, right, bottom);
    tgi_line(right, bottom, left, bottom);
    tgi_line(left, bottom, left, top);

    /* Patrón regular de rombos dentro del cuadrado. */
    for (y = top + 12; y < bottom - 8; y += 24) {
        for (x = left + 12; x < right - 8; x += 32) {
            diamond(x + ((y / 24) & 1) * 16, y, 9);
        }
    }

    draw_text_bitmap(232, 6, "QUAD VDC 80 COLUMNAS 640X200", 2);
    draw_text_bitmap(230, 180, "PULSA UNA TECLA", 2);
}

static void restore_text_mode(void)
{
    tgi_done();
    tgi_uninstall();
    videomode(VIDEOMODE_80COL);
    clrscr();
    cursor(1);
    cputs("quad: modo grafico finalizado.\r\n");
    cputs("El modo texto VDC fue restaurado.\r\n");
}

int main(void)
{
    videomode(VIDEOMODE_80COL);
    clrscr();
    cursor(1);
    cputs("quad - prueba grafica VDC de 80 columnas\r\n");
    cputs("Cargando el controlador c128-vdc.tgi...\r\n");

    cputs("Activando el TGI integrado en el programa...\r\n");
    tgi_install(tgi_static_stddrv);
    if (check_tgi("tgi_install") != TGI_ERR_OK) {
        abort_tgi();
        return 1;
    }
    cputs("TGI cargado correctamente.\r\n");
    cputs("Ingresando al modo grafico...\r\n");

    tgi_init();
    if (check_tgi("tgi_init") != TGI_ERR_OK) {
        abort_tgi();
        return 1;
    }

    tgi_setpalette(palette);
    if (check_tgi("tgi_setpalette") != TGI_ERR_OK) {
        tgi_done();
        abort_tgi();
        return 1;
    }

    /* No permitimos que una tecla anterior salte la espera final. */
    while (kbhit()) cgetc();
    draw_quad();
    cgetc();
    restore_text_mode();
    return 0;
}
