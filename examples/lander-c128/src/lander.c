#include <c128.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <tgi.h>

/* Port nativo C128 del juego DOS original. Usa c128-vdc.tgi: 640x200, 2 colores, 16 KB VDC. */
#define SCREEN_W 640
#define SCREEN_H 200
#define GROUND_Y 175
#define PLATFORM_X1 500
#define PLATFORM_X2 600
#define PLATFORM_Y 155
#define LANDER_W 50
#define LANDER_H 50
#define GROUND_POINTS 23
#define TEST_NO_BACKGROUND_RESTORE 1
#define TEST_NO_BACKGROUND_CAPTURE 1

typedef struct { int x; int y; } Star;

static const Star stars[] = {
    {18,12},{53,31},{91,8},{127,45},{161,20},{194,57},{231,16},{267,38},
    {302,9},{341,51},{376,27},{411,6},{447,42},{482,18},{519,56},{553,29},
    {590,11},{625,47},{36,73},{78,102},{116,84},{154,128},{205,93},{248,116},
    {286,76},{326,137},{365,103},{405,71},{438,124},{476,91},{532,108},
    {575,78},{612,133},{67,145},{181,151},{291,148},{457,145},{604,153}
};

static int x_lander = 50;
static int y_lander = 35;
static int x_meteor = SCREEN_W - LANDER_W;
static int y_meteor = 70;
static unsigned seed = 0x128;
static int ground_y[GROUND_POINTS];
static const unsigned char mono_palette[2] = {
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

static void white(void);

static const unsigned char glyph_dot[7] = {0,0,0,0,0,0,4};
static const unsigned char glyph_dash[7] = {0,0,0,31,0,0,0};
static const unsigned char glyph_underscore[7] = {0,0,0,0,0,0,31};
static const unsigned char glyph_blank[7] = {0,0,0,0,0,0,0};

static const unsigned char* glyph_for(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return font5x7[c - 'A'];
    if (c >= '0' && c <= '9') return digits5x7[c - '0'];
    if (c == '.') return glyph_dot;
    if (c == '-') return glyph_dash;
    if (c == '_') return glyph_underscore;
    return glyph_blank;
}

static void draw_text_bitmap(int x, int y, const char* text, unsigned char scale)
{
    unsigned char c, row, bit, i;
    const unsigned char* glyph;
    white();
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

static int rnd(int maximum)
{
    seed = seed * 109u + 89u;
    return (int)((seed >> 8) % (unsigned)maximum);
}

static void white(void) { tgi_setcolor(1); }

static unsigned char check_tgi(const char* operation)
{
    unsigned char error = tgi_geterror();
    if (error != TGI_ERR_OK) {
        videomode(VIDEOMODE_80COL);
        cprintf("TGI fallo en %s: %u\r\n", operation, error);
        cputs("Pulsa una tecla para volver a BASIC.\r\n");
        cgetc();
    }
    return error;
}

static void abort_tgi(void)
{
    tgi_uninstall();
    videomode(VIDEOMODE_80COL);
    clrscr();
    cursor(1);
    cputs("Lander Oscar: TGI descargado correctamente.\r\n");
}

static void draw_stars(void)
{
    unsigned i;
    white();
    for (i = 0; i < sizeof(stars) / sizeof(stars[0]); ++i)
        tgi_setpixel(stars[i].x, stars[i].y);
}

static void draw_ground(void)
{
    int x;
    white();
    for (x = 0; x < GROUND_POINTS - 1; ++x) {
        tgi_line(x * 30, ground_y[x], (x + 1) * 30, ground_y[x + 1]);
    }
    tgi_bar(PLATFORM_X1, PLATFORM_Y, PLATFORM_X2, PLATFORM_Y + 8);
}

static void init_ground(void)
{
    unsigned char i;
    ground_y[0] = GROUND_Y;
    for (i = 1; i < GROUND_POINTS - 1; ++i)
        ground_y[i] = GROUND_Y - 10 + rnd(21);
    ground_y[GROUND_POINTS - 1] = GROUND_Y;
}

typedef struct { unsigned char x1, y1, x2, y2; } SpriteSegment;
typedef struct { const SpriteSegment* segments; unsigned char count; } Sprite;

#define SPRITE_SIZE 51
static unsigned char lander_background[SPRITE_SIZE * SPRITE_SIZE];
static unsigned char meteor_background[SPRITE_SIZE * SPRITE_SIZE];
static unsigned char explosion_background[SPRITE_SIZE * SPRITE_SIZE];

static const SpriteSegment lander_segments[] = {
    {15,1,20,0},{20,0,30,0},{30,0,35,1},{12,3,38,3},
    {10,6,40,6},{9,9,41,9},{10,12,40,12},{12,16,38,16},
    {10,21,40,21},{10,22,40,35},{11,36,39,36},{15,37,35,37},
    {15,38,10,50},{35,38,40,50},{20,37,30,37},{20,38,30,42},
    {20,42,30,42}
};
static const SpriteSegment meteor_segments[] = {
    {15,10,35,10},{10,13,40,13},{6,17,44,17},{3,22,47,22},
    {1,27,49,27},{3,32,47,32},{6,37,44,37},{10,42,40,42},
    {15,45,35,45},{11,17,17,17},{12,16,12,22},{31,27,37,27},
    {34,26,34,31}
};
static const SpriteSegment explosion1_segments[] = {
    {25,0,25,50},{0,25,50,25},{7,7,43,43},{43,7,7,43},
    {15,3,35,47},{3,15,47,35},{12,12,38,38},{38,12,12,38}
};
static const SpriteSegment explosion2_segments[] = {
    {25,10,25,40},{10,25,40,25},{15,15,35,35},{35,15,15,35},
    {20,12,30,38},{12,20,38,30}
};
static const Sprite sprite_lander = { lander_segments, sizeof(lander_segments) / sizeof(lander_segments[0]) };
static const Sprite sprite_meteor = { meteor_segments, sizeof(meteor_segments) / sizeof(meteor_segments[0]) };
static const Sprite sprite_explosion1 = { explosion1_segments, sizeof(explosion1_segments) / sizeof(explosion1_segments[0]) };
static const Sprite sprite_explosion2 = { explosion2_segments, sizeof(explosion2_segments) / sizeof(explosion2_segments[0]) };

static void draw_sprite(const Sprite* sprite, int x, int y)
{
    unsigned char i;
    white();
    for (i = 0; i < sprite->count; ++i)
        tgi_line(x + sprite->segments[i].x1, y + sprite->segments[i].y1,
                 x + sprite->segments[i].x2, y + sprite->segments[i].y2);
}
static void draw_lander(int x, int y) { draw_sprite(&sprite_lander, x, y); }
static void draw_meteor(int x, int y) { draw_sprite(&sprite_meteor, x, y); }
static void draw_explosion(int x, int y, unsigned char stage)
{
    draw_sprite(stage == 0 ? &sprite_explosion1 : &sprite_explosion2, x, y);
}

static void capture_background(int x, int y, unsigned char* buffer)
{
    unsigned char row, col;
    for (row = 0; row < SPRITE_SIZE; ++row) {
        for (col = 0; col < SPRITE_SIZE; ++col) {
            if (x + col < 0 || x + col >= SCREEN_W || y + row < 0 || y + row >= SCREEN_H)
                buffer[(unsigned)row * SPRITE_SIZE + col] = 0;
            else
                buffer[(unsigned)row * SPRITE_SIZE + col] =
                    (tgi_getpixel(x + col, y + row) != TGI_COLOR_BLACK);
        }
    }
}

static void restore_background(int x, int y, const unsigned char* buffer)
{
    unsigned char row, col;
    for (row = 0; row < SPRITE_SIZE; ++row) {
        for (col = 0; col < SPRITE_SIZE; ++col) {
            if (x + col >= 0 && x + col < SCREEN_W && y + row >= 0 && y + row < SCREEN_H) {
                tgi_setcolor(buffer[(unsigned)row * SPRITE_SIZE + col] ?
                             TGI_COLOR_WHITE : TGI_COLOR_BLACK);
                tgi_setpixel(x + col, y + row);
            }
        }
    }
}

static void draw_scene(void)
{
    tgi_clear();
    draw_stars();
    draw_ground();
#if !TEST_NO_BACKGROUND_CAPTURE
    capture_background(x_lander, y_lander, lander_background);
    capture_background(x_meteor, y_meteor, meteor_background);
#endif
    draw_lander(x_lander, y_lander);
    draw_meteor(x_meteor, y_meteor);
}

static void message(const char* text)
{
    draw_text_bitmap(150, 90, text, 1);
    cgetc();
}

static void wait_for_new_key(void)
{
    /* Descarta cualquier tecla que haya quedado en el buffer al arrancar
       desde BASIC y espera una pulsacion nueva para que la ayuda permanezca
       visible el tiempo suficiente. */
    while (kbhit()) cgetc();
    while (!kbhit()) { }
    cgetc();
}

static void explode(void)
{
    unsigned i;
#if !TEST_NO_BACKGROUND_RESTORE
    restore_background(x_lander, y_lander, lander_background);
#endif
#if !TEST_NO_BACKGROUND_CAPTURE
    capture_background(x_lander, y_lander, explosion_background);
#endif
    for (i = 0; i < 5; ++i) {
        draw_explosion(x_lander, y_lander, 0);
#if !TEST_NO_BACKGROUND_RESTORE
        restore_background(x_lander, y_lander, explosion_background);
#endif
        draw_explosion(x_lander, y_lander, 1);
#if !TEST_NO_BACKGROUND_RESTORE
        restore_background(x_lander, y_lander, explosion_background);
#endif
    }
}

static void restore_text_mode(void)
{
    tgi_done();
    tgi_uninstall();
    videomode(VIDEOMODE_80COL);
    clrscr();
    cursor(1);
    cputs("Lander Oscar: modo grafico finalizado.\r\n");
    cputs("El modo texto VDC fue restaurado.\r\n");
}

static void game(void)
{
    unsigned char key;
    unsigned char finished = 0;

    cursor(0);
    /* RUN puede reutilizar la memoria del programa anterior; reiniciamos
       explícitamente el estado para que cada partida empiece igual. */
    x_lander = 50;
    y_lander = 35;
    x_meteor = SCREEN_W - LANDER_W;
    y_meteor = 70;
    seed = 0x128;
    init_ground();
    draw_scene();
    while (!finished) {
#if !TEST_NO_BACKGROUND_RESTORE
        restore_background(x_lander, y_lander, lander_background);
        restore_background(x_meteor, y_meteor, meteor_background);
#endif

        if (kbhit()) {
            key = cgetc();
            if (key == CH_ESC) finished = 1;
            else if (key == CH_CURS_LEFT || key == 'a' || key == 'A') x_lander -= 5;
            else if (key == CH_CURS_RIGHT || key == 'd' || key == 'D') x_lander += 5;
            else if (key == CH_CURS_UP || key == 'w' || key == 'W') y_lander -= 5;
            else if (key == CH_CURS_DOWN || key == 's' || key == 'S') y_lander += 5;
        }

        if (x_lander < 0) x_lander = 0;
        if (x_lander > SCREEN_W - LANDER_W) x_lander = SCREEN_W - LANDER_W;
        if (y_lander < 0) y_lander = 0;
        /* El original permitia que el sprite rebasara el borde inferior. */
        if (y_lander > SCREEN_H) y_lander = SCREEN_H;
        y_lander += rnd(2);

        x_meteor -= 10;
        if (x_meteor < -LANDER_W) {
            x_meteor = SCREEN_W;
            y_meteor = 10 + rnd(GROUND_Y - LANDER_H - 10);
        }

#if !TEST_NO_BACKGROUND_CAPTURE
        capture_background(x_lander, y_lander, lander_background);
        capture_background(x_meteor, y_meteor, meteor_background);
#endif
        draw_meteor(x_meteor, y_meteor);
        draw_lander(x_lander, y_lander);

        if (y_lander >= PLATFORM_Y && y_lander < GROUND_Y &&
            x_lander + LANDER_W >= PLATFORM_X1 && x_lander <= PLATFORM_X2) {
        message("ATERRIZAJE LOGRADO - TECLA PARA CONTINUAR");
            finished = 1;
        } else if (y_lander >= GROUND_Y) {
            explode();
        message("EL LANDER SE ESTRELLO - TECLA PARA CONTINUAR");
            finished = 1;
        } else if (x_lander < x_meteor + LANDER_W && x_lander + LANDER_W > x_meteor &&
                   y_lander < y_meteor + LANDER_H && y_lander + LANDER_H > y_meteor) {
            explode();
        message("IMPACTO CON METEORITO - TECLA PARA CONTINUAR");
            finished = 1;
        }
    }
}

int main(void)
{
    videomode(VIDEOMODE_80COL);
    clrscr();
    cursor(1);
    cputs("Lander Oscar C128\r\n");
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
    tgi_setpalette(mono_palette);
    if (check_tgi("tgi_setpalette") != TGI_ERR_OK) {
        tgi_done();
        abort_tgi();
        return 1;
    }
    tgi_clear();
    draw_text_bitmap(8, 4, "LANDER OSCAR C128 VDC MONOCROMO", 1);
    draw_text_bitmap(120, 35, "FLECHAS W A S D PARA PILOTAR", 1);
    draw_text_bitmap(180, 50, "ESC PARA ABANDONAR", 1);
    draw_text_bitmap(150, 80, "INICIANDO", 1);
    draw_text_bitmap(140, 100, "PRESIONE TECLA", 1);
    wait_for_new_key();
    /* Evita quedar bloqueado en una pantalla negra si la entrada llega tarde. */
    /* El driver c128-vdc no necesita waitvsync(); el redibujado del cuadro
       ya sincroniza naturalmente el bucle y evita bloquear la ejecución. */
    game();
    restore_text_mode();
    return 0;
}
