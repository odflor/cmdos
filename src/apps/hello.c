#include <conio.h>
#include "../kernel/tui.h"

/* First external CMD-OS application prototype. */
int main(void)
{
    cputs("\r\nHELLO: aplicacion externa CMD-OS\r\n");
    cputs("Este programa fue cargado sin stub de BASIC.\r\n");
    cputs("El control regresara al interprete al terminar.\r\n");
    TUI_REQUEST.operation = TUI_ARGUMENTS;
    ((void (*)(void))0x0E00)();
    if (TUI_REQUEST.data[0]) {
        cputs("Argumentos: ");
        cputs((const char*)TUI_REQUEST.data);
        cputs("\r\n");
    }
    cputs("Presione una tecla para finalizar...\r\n");
    TUI_REQUEST.operation = TUI_KEY;
    ((void (*)(void))0x0E00)();
    return 0;
}
