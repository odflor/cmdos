#include <conio.h>

/* First external CMD-OS application prototype. */
void main(void)
{
    cputs("\r\nHELLO: aplicacion externa CMD-OS\r\n");
    cputs("Este programa fue cargado sin stub de BASIC.\r\n");
    cputs("El control regresara al interprete al terminar.\r\n");
    cputs("Presione una tecla para finalizar...\r\n");
    cgetc();
}
