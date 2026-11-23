/*
 * Vibe Coding para C128
 * Hola Mundo en modo 80 columnas
 */
#include <stdio.h>
#include <conio.h>
#include <c128.h>

void main(void) {
    // Cambiar a modo 80 columnas (VDC)
    videomode(VIDEOMODE_80x25);
    
    clrscr();
    textcolor(COLOR_GREEN);
    revers(1);
    /* En la consola C128 usamos CR+LF para volver también a la columna 0. */
    printf(" MODO VIBE CODING ACTIVADO: COMMODORE 128 \r\n");
    revers(0);
    
    textcolor(COLOR_WHITE);
    printf("\r\nEsperando instrucciones de disenho...\r\n");
    printf("\r\nEstado del sistema:\r\n");
    printf("- CPU: 8502 (Modo 128)\r\n");
    printf("- Video: VDC 80 Columnas\r\n");
    printf("- RAM: 128 KB\r\n");

    while(1) {
        // Bucle infinito para mantener la pantalla
    }
}
