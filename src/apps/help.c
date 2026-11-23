#include <conio.h>

int main(void)
{
    cputs("\r\nCommand-OS - HELP\r\n\r\n"
          "A: B: C: D:      Selecciona la unidad IEC 8, 9, 10 u 11.\r\n"
          "M:               Selecciona el volumen en la REU si esta montado.\r\n"
          "FORMAT M: [/F]   Inicializa M:. /F permite reformatear. Pide confirmacion.\r\n"
          "DIR              Lista archivos, tipos, tamanos y espacio de la unidad.\r\n"
          "RUN nombre [args] Ejecuta un CMDP de la unidad activa con sus argumentos.\r\n"
          "nombre [args]     Ejecuta el CMDP sin escribir RUN. La extension .cmd es\r\n"
          "                 opcional. Ejemplo: edit\r\n"
          "COPY origen destino  Copia PRG, SEQ o USR sin reemplazar el destino.\r\n"
          "                 Indique las dos unidades: COPY A:edit.cmd M:edit.cmd\r\n"
          "DEL nombre       Borra el archivo indicado en la unidad activa.\r\n"
          "STATUS           Muestra el ultimo codigo devuelto por un programa.\r\n"
          "VER              Muestra la version de CMD-OS.\r\n"
          "EXIT             Avisa si hay sesiones suspendidas antes de apagar.\r\n"
          "HELP             Muestra los comandos. Tambien funciona la tecla HELP.\r\n"
          "\r\n"
          "Aplicaciones incluidas: hello (demostracion), mem (mapa de memoria)\r\n"
          "y edit [archivo] (abre un SEQ o prepara un documento nuevo).\r\n"
          "Los nombres distinguen mayusculas y minusculas y se escriben sin espacios.\r\n"
          "\r\nEnter: editor y sesiones    Esc: volver");
    for (;;) {
        unsigned char key = cgetc();
        if (key == CH_ESC) {
            cputs("\r\n");
            return 0;
        }
        if (key == CH_ENTER) break;
    }

    clrscr();
    cputs("Command-OS - EDITOR Y SESIONES\r\n\r\n"
          "EDIT [archivo]   Abre un SEQ o propone ese nombre al guardar.\r\n"
          "Sin extension, EDIT agrega .txt. Los documentos se guardan como SEQ.\r\n"
          "F1 Abre          Enter sin nombre muestra el selector de archivos.\r\n"
          "F2 Inicio        F3 Guardar        F4 Fin        F7 Menu\r\n"
          "Guardar como     Se elige desde el menu Archivo.\r\n"
          "\r\n"
          "Alt+Tab          Abre el menu de sesiones con M: montado.\r\n"
          "Ctrl+Y           En VICE, si el anfitrion captura Alt+Tab.\r\n"
          "Interprete       Suspende EDIT y vuelve a la consola.\r\n"
          "Reanudar         Selecciona una sesion con las flechas y pulsa Enter.\r\n"
          "Descartar        Libera una sesion suspendida despues de confirmar.\r\n"
          "Esc              Cierra el menu y vuelve a la pantalla activa.\r\n"
          "\r\n"
          "Cierra los canales de disco antes de cambiar de sesion.\r\n"
          "Si la REU conserva sus datos al reiniciar, se pueden reanudar sesiones.\r\n"
          "\r\nPulsa una tecla para volver.\r\n");
    cgetc();
    return 0;
}
