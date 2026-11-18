#include <stdio.h>
#include <conio.h>
#include <c128.h>

#define WIDTH  80
#define HEIGHT 25

/* Caracteres para construir la textura geométrica de la mandala */
const char symbols[] = { ' ', '.', ':', '-', '=', '+', '*', '#', '%', '@' };
#define NUM_SYMBOLS 10

void main(void) {
    int x, y;
    int dx, dy;
    int dist2, pattern;
    unsigned char color;
    unsigned char idx;

    /* 1. Activar el modo de 80 columnas de la C128 (VDC 8563) */
    videomode(VIDEOMODE_80x25);

    /* 2. Configurar colores y limpiar pantalla */
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();

    /* 3. Bucle de renderizado por coordenadas (X, Y) */
    for (y = 0; y < HEIGHT; ++y) {
        for (x = 0; x < WIDTH; ++x) {
            
            /* Centrar coordenadas (Centro aproximado: X=40, Y=12) */
            dx = x - 40;
            dy = (y - 12) * 2; /* Factor 2 para compensar la altura de las celdas de texto */

            /* Cálculo de distancia al cuadrado y simetría cruzada con enteros */
            dist2 = (dx * dx) + (dy * dy);
            
            /* Fórmula demoscene basada en bits y álgebra entera para crear simetría radial */
            pattern = (dist2 >> 5) ^ (dx * dy >> 3);

            /* Mapear el resultado al rango de nuestros caracteres */
            idx = (unsigned char)(pattern % NUM_SYMBOLS);
            if (idx > (NUM_SYMBOLS - 1)) {
                idx = 0;
            }

            /* Asignar un color dinámico cíclico basado en la posición */
            color = ((x + y) % 14) + 1;

            /* Dibujar en el VDC de 80 columnas */
            gotoxy(x, y);
            textcolor(color);
            cputc(symbols[idx]);
        }
    }

    /* Título superior en la pantalla de 80 columnas */
    gotoxy(25, 0);
    textcolor(COLOR_WHITE);
    cputs(" MANDALA C128 - 80 COLUMNAS ");

    /* Pausa para mantener la imagen en pantalla hasta presionar una tecla */
    cgetc();
}