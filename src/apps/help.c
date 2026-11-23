#include <conio.h>

int main(void)
{
    cputs("\r\nCommand-OS - HELP\r\n\r\n"
          "A: B: C: D:      Selecciona la unidad IEC 8, 9, 10 u 11.\r\n"
          "M:               Selecciona el volumen en la REU si esta montado.\r\n"
          "FORMAT M:        Inicializa M: si no esta montado. Pide confirmacion.\r\n"
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
          "Los nombres distinguen mayusculas y minusculas y se escriben sin espacios.\r\n");
    return 0;
}
