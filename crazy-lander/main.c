/*
 * Crazy Lander - primer esqueleto para Commodore 128
 *
 * Objetivo de esta etapa:
 *   - Ejecutable .PRG autocontenido para el target c128 de CC65.
 *   - Modo 40 columnas, usando el VIC-II.
 *   - Máquina de estados lista para presentación, juego y score.
 *
 * Los módulos de gráficos, sprites y SID se incorporarán en las siguientes
 * etapas sin cambiar el formato de distribución: un único CRAZYLANDER.PRG.
 */
#include <conio.h>
#include <c128.h>
#include <cbm.h>
#include <stdio.h>
#include <string.h>

#define sCR "\r\n"
#define SPRITE_MEMORY 0x3000
#define SPRITE_POINTER 0xC0
#define LANDER_X 140
#define LANDER_START_Y 76
/* La superficie está en la fila 23; el sprite mide 21 píxeles de alto. */
#define LANDER_BOTTOM_Y 213
#define GRAVITY 0x0001
#define THRUST 0x0018
#define HORIZONTAL_STEP 2

typedef enum {
    SCREEN_TITLE,
    SCREEN_GAME,
    SCREEN_SCORE
} Screen;

/* Sprite 0: pequeño módulo lunar de 24 x 21 píxeles. */
static const unsigned char lander_sprite[64] = {
    0x00, 0x18, 0x00,
    0x00, 0x3C, 0x00,
    0x00, 0x7E, 0x00,
    0x00, 0xFF, 0x00,
    0x03, 0xFF, 0xC0,
    0x07, 0xFF, 0xE0,
    0x0F, 0xFF, 0xF0,
    0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8,
    0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC,
    0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8,
    0x0F, 0xFF, 0xF0,
    0x0C, 0x3C, 0x30,
    0x0C, 0x3C, 0x30,
    0x18, 0x18, 0x18,
    0x18, 0x18, 0x18,
    0x30, 0x00, 0x0C,
    0x60, 0x00, 0x06,
    0x00
};

static void hide_lander(void)
{
    VIC.spr_ena &= 0xFE;
}

static void install_lander(void)
{
    /* La pantalla estándar está en el banco VIC 0; $3000 no pisa el texto. */
    memcpy((void*)SPRITE_MEMORY, lander_sprite, sizeof(lander_sprite));
    *((unsigned char*)0x07F8) = SPRITE_POINTER;
    VIC.spr_color[0] = COLOR_YELLOW;
    VIC.spr_pos[0].x = LANDER_X;
    VIC.spr_pos[0].y = LANDER_START_Y;
    VIC.spr_hi_x &= 0xFE;
    VIC.spr_ena |= 0x01;
}

static void move_lander(unsigned int x, unsigned char y)
{
    VIC.spr_pos[0].x = (unsigned char)x;
    VIC.spr_pos[0].y = y;
    if (x > 255) {
        VIC.spr_hi_x |= 0x01;
    } else {
        VIC.spr_hi_x &= 0xFE;
    }
}

static void show_velocity(int velocity)
{
    gotoxy(25, 0);
    cprintf("VEL:%3d", velocity >> 4);
}

static void show_altitude(int position)
{
    int altitude = LANDER_BOTTOM_Y - position;

    if (altitude < 0) {
        altitude = 0;
    }
    gotoxy(0, 1);
    cprintf("ALT:%3d", altitude);
}

static void show_title(void)
{
    clrscr();
    textcolor(COLOR_LIGHTBLUE);
    cprintf("***************************************" sCR);
    cprintf("*          C R A Z Y  L A N D E R     *" sCR);
    cprintf("***************************************" sCR sCR);

    textcolor(COLOR_WHITE);
    cprintf("  ATERRIZA. SOBREVIVE. NO TE QUEDES" sCR);
    cprintf("  SIN COMBUSTIBLE." sCR sCR);
    cprintf("  [ESPACIO] COMENZAR" sCR);
    cprintf("  [A]/[D]   MOVER IZQ./DER." sCR);
    cprintf("  [W]       FRENAR / ELEVAR" sCR);
    cprintf("  [L]       VER SCORE" sCR);
    cprintf("  [Q]       SALIR" sCR);
}

static void show_game(int velocity, int position)
{
    clrscr();
    textcolor(COLOR_WHITE);
    /* velocity está en punto fijo; se muestra una escala legible. */
    cprintf("FUEL: 100%% SCORE: 000000" sCR);
    show_velocity(velocity);
    show_altitude(position);
    gotoxy(0, 23);
    cprintf("----------------------------------------");
    gotoxy(0, 24);
    cprintf("[A] IZQ.  [D] DER.  [W] FRENAR/ELEVAR");
}

static void show_score(void)
{
    clrscr();
    textcolor(COLOR_YELLOW);
    cprintf("              MISION TERMINADA" sCR sCR);
    textcolor(COLOR_WHITE);
    cprintf("              SCORE: 000000" sCR sCR);
    cprintf("              [ESPACIO] otra partida" sCR);
    cprintf("              [L]       ver score" sCR);
    cprintf("              [Q]       salir" sCR);
}

void main(void)
{
    Screen screen = SCREEN_TITLE;
    unsigned char key;
    int velocity;
    int position;
    unsigned int lander_x;

    /* El juego usa el VIC-II en la pantalla estándar de 40 columnas. */
    videomode(VIDEOMODE_40x25);
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLUE);
    cursor(0);
    hide_lander();

    while (1) {
        if (screen == SCREEN_TITLE) {
            show_title();
            key = cgetc();
            if (key == 'q' || key == 'Q') {
                break;
            }
            if (key == ' ') {
                screen = SCREEN_GAME;
            } else if (key == 'l' || key == 'L') {
                screen = SCREEN_SCORE;
            }
        } else if (screen == SCREEN_GAME) {
            lander_x = LANDER_X;
            position = LANDER_START_Y;
            velocity = 0x0040;
            show_game(velocity, position);
            install_lander();

            while (screen == SCREEN_GAME && position < LANDER_BOTTOM_Y) {
                if (kbhit()) {
                    key = cgetc();
                    if (key == 'q' || key == 'Q') {
                        screen = SCREEN_TITLE;
                        break;
                    }
                    if (key == 'a' || key == 'A') {
                        if (lander_x > 24) {
                            lander_x -= HORIZONTAL_STEP;
                        }
                    }
                    if (key == 'd' || key == 'D') {
                        if (lander_x < 296) {
                            lander_x += HORIZONTAL_STEP;
                        }
                    }
                    if (key == 'w' || key == 'W') {
                        /* Empuje negativo: puede vencer la gravedad y subir. */
                        velocity -= THRUST;
                        if (velocity < -0x0200) {
                            velocity = -0x0200;
                        }
                    }
                }

                move_lander(lander_x, (unsigned char)position);
                waitvsync();
                position += velocity >> 8;
                if (position < 20) {
                    position = 20;
                    velocity = 0;
                }
                if (velocity < 0x0300) {
                    velocity += GRAVITY;
                }
                show_velocity(velocity);
                show_altitude(position);
            }

            hide_lander();
            if (screen == SCREEN_GAME) {
                clrscr();
                if (velocity > 0x0100) {
                    textcolor(COLOR_RED);
                    cprintf("              K A B O O M !" sCR sCR);
                    cprintf("        EL LANDER SE ESTRELLO." sCR);
                } else {
                    textcolor(COLOR_GREEN);
                    cprintf("             ATERRIZAJE EXITOSO" sCR sCR);
                }
                textcolor(COLOR_WHITE);
                cprintf("              [L] ver score" sCR);
                cprintf("              [ESPACIO] continuar" sCR);
                cprintf("              [Q] salir" sCR);
                key = cgetc();
                if (key == 'q' || key == 'Q') {
                    break;
                }
                if (key == 'l' || key == 'L') {
                    screen = SCREEN_SCORE;
                } else {
                    screen = SCREEN_TITLE;
                }
            }
        } else {
            hide_lander();
            show_score();
            key = cgetc();
            if (key == 'q' || key == 'Q') {
                break;
            }
            if (key == ' ') {
                screen = SCREEN_GAME;
            }
        }
    }

    cursor(1);
    clrscr();
}
