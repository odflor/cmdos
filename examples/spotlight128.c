#include <conio.h>
#include <c128.h>
#include <accelerator.h>

#define VDC_REGISTER       (*(volatile unsigned char*)0xD600)
#define VDC_DATA           (*(volatile unsigned char*)0xD601)
#define VDC_READY          0x80
#define VDC_VBLANK         0x20
#define VIEW_WIDTH         40
#define LAST_OFFSET        40
#define FRAMES_PER_STEP    6

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

static unsigned int vdc_read_address(unsigned char high_reg)
{
    unsigned int address;

    address = (unsigned int)vdc_read_register(high_reg) << 8;
    address |= vdc_read_register(high_reg + 1);
    return address;
}

static void vdc_write_address(unsigned char high_reg, unsigned int address)
{
    vdc_write_register(high_reg, (unsigned char)(address >> 8));
    vdc_write_register(high_reg + 1, (unsigned char)address);
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

static void wait_one_frame(void)
{
    /* Wait for the next VDC vertical blank and its end. */
    while ((VDC_REGISTER & VDC_VBLANK) != 0) {
    }
    while ((VDC_REGISTER & VDC_VBLANK) == 0) {
    }
    while ((VDC_REGISTER & VDC_VBLANK) != 0) {
    }
}

static unsigned char wait_frames(unsigned char count)
{
    while (count != 0) {
        wait_one_frame();
        if (kbhit()) {
            return 1;
        }
        --count;
    }
    return 0;
}

static void set_view(unsigned int screen_base, unsigned int attr_base,
                     unsigned char offset)
{
    vdc_write_address(12, screen_base + offset);
    vdc_write_address(20, attr_base + offset);
}

static void vdc_set_40_column_view(void)
{
    vdc_write_register(0, 63);       /* Half-width horizontal total */
    vdc_write_register(1, VIEW_WIDTH);
    vdc_write_register(2, 55);      /* Centered VDC 40-column position */
    vdc_write_register(22, 137);    /* Double-width character pixels */
    vdc_write_register(25, vdc_read_register(25) | 0x10);
    vdc_write_register(27, 40);     /* 40 visible + 40 skipped = 80 stride */
}

void main(void)
{
    unsigned char saved_r0;
    unsigned char saved_r1;
    unsigned char saved_r2;
    unsigned char saved_r22;
    unsigned char saved_r25;
    unsigned char saved_r27;
    unsigned char saved_cursor;
    unsigned char saved_speed;
    unsigned char end_pause_frames;
    unsigned char offset;
    signed char direction;
    unsigned int screen_base;
    unsigned int attr_base;

    videomode(VIDEOMODE_80x25);
    saved_speed = get_c128_speed();
    set_c128_speed(SPEED_SLOW);
    /* Standard KERNAL R7 is 29 on NTSC and 32 on PAL. */
    end_pause_frames = (vdc_read_register(7) == 29) ? 30 : 25;

    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    revers(0);
    clrscr();

    /* Remember the standard 80-column setup so BASIC can get it back. */
    saved_r0 = vdc_read_register(0);
    saved_r1 = vdc_read_register(1);
    saved_r2 = vdc_read_register(2);
    saved_r22 = vdc_read_register(22);
    saved_r25 = vdc_read_register(25);
    saved_r27 = vdc_read_register(27);
    screen_base = vdc_read_address(12);
    attr_base = vdc_read_address(20);
    saved_cursor = cursor(0);

    /* Fill the complete 80-column screen before narrowing the VDC window. */
    put_text(0, 1, COLOR_CYAN, 1,
             "SPOTLIGHT 128: RECORRIDO HORIZONTAL POR EL VDC");
    put_text(0, 3, COLOR_YELLOW, 0,
             "IZQUIERDA: texto desde la columna cero.");
    put_text(40, 3, COLOR_LIGHTBLUE, 0,
             "DERECHA: contenido oculto hasta mover el visor.");

    put_text(0, 5, COLOR_WHITE, 0,
             "80 columnas en memoria; ventana de 40.");
    put_text(40, 5, COLOR_LIGHTGREEN, 0,
             "Solo ves cuarenta; mueve el visor para explorar.");

    put_text(0, 7, COLOR_LIGHTRED, 0,
             "COLORES: mira como cambian las palabras.");
    put_text(40, 7, COLOR_WHITE, 0,
             "En la mitad derecha: ");
    put_text(49, 7, COLOR_YELLOW, 0, "COLOR");
    put_text(55, 7, COLOR_CYAN, 0, "VENTANA");

    put_text(0, 9, COLOR_GREEN, 0,
             "Lorem ipsum dolor sit amet, consectetur");
    put_text(40, 9, COLOR_LIGHTRED, 0,
             "adipiscing elit, sed do eiusmod tempor.");

    put_text(0, 11, COLOR_WHITE, 0,
             "Cada paso mueve el inicio de la pantalla una columna.");
    put_text(0, 13, COLOR_LIGHTBLUE, 0,
             "Mira como el texto de la derecha entra en el visor.");
    put_text(0, 16, COLOR_CYAN, 1,
             "PULSE CUALQUIER TECLA PARA TERMINAR EL RECORRIDO");

    vdc_set_40_column_view();

    /* Move the 40-column viewport across the 80-column rows and back. */
    offset = 0;
    direction = 1;
    while (!kbhit()) {
        set_view(screen_base, attr_base, offset);

        if (wait_frames(FRAMES_PER_STEP)) {
            goto finished;
        }

        if (offset == 0 || offset == LAST_OFFSET) {
            if (wait_frames(end_pause_frames)) {
                goto finished;
            }
        }

        if (offset == LAST_OFFSET) {
            direction = -1;
        } else if (offset == 0) {
            direction = 1;
        }
        offset = (unsigned char)(offset + direction);
    }

finished:
    if (kbhit()) {
        cgetc();
    }

    /* Restore the VDC mode and its original text/attribute memory origins. */
    vdc_write_register(0, saved_r0);
    vdc_write_register(1, saved_r1);
    vdc_write_register(2, saved_r2);
    vdc_write_register(22, saved_r22);
    vdc_write_register(25, saved_r25);
    vdc_write_register(27, saved_r27);
    vdc_write_address(12, screen_base);
    vdc_write_address(20, attr_base);
    cursor(saved_cursor);
    set_c128_speed(saved_speed);
}
