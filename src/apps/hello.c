#include <conio.h>
#include "../kernel/cmdosk-abi.h"

/* First external CMD-OS application prototype. */
int main(void)
{
    cputs("\r\nHELLO: aplicacion externa CMD-OS\r\n");
    cputs("Este programa fue cargado sin stub de BASIC.\r\n");
    cputs("El control regresara al interprete al terminar.\r\n");
    KRNL_REQUEST.operation = KRNL_ARGS;
    ((void (*)(void))0x0E00)();
    if (KRNL_REQUEST.data[0]) {
        cputs("Argumentos: ");
        cputs((const char*)KRNL_REQUEST.data);
        cputs("\r\n");
    }
    cputs("Presione una tecla para finalizar...\r\n");
    KRNL_REQUEST.operation = KRNL_KEY_READ;
    ((void (*)(void))0x0E00)();
    return 0;
}
