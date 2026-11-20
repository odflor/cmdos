# CMD-OS

## Resumen ejecutivo

CMD-OS es un entorno al estilo DOS para el Commodore 128 en modo nativo. Arranca
desde un disquete D64 y usa la pantalla VDC de 80 columnas con texto cian sobre
negro. `dir` lista la unidad activa, `run nombre` o `nombre` ejecuta una
aplicación `.cmd`, y `exit` devuelve el control a BASIC 7.0.

El intérprete y el kernel permanecen en el banco 0 y reanudan la consola al
terminar cada aplicación. CMD-OS ejecuta una aplicación a la vez. `mem` muestra
el mapa de memoria del banco 0. `edit` usa el banco 1 para documentos SEQ de
hasta 34 KiB y ofrece menús desplegables, diálogos y guardado con respaldo al
reemplazar un archivo existente.

La API común ofrece servicios de pantalla, teclado y disco. En los diálogos de
archivo, Enter con el nombre vacío muestra una lista filtrada por el tipo que
corresponde. Las flechas y Enter permiten elegir, y Esc cancela. Los archivos por
lotes y el disco virtual `M:` siguen pendientes.

## Próxima etapa

El formato de referencia pasará a ser D71 para la 1571. CMD-OS arrancará
directamente desde el dispositivo 8 mediante el mecanismo de autoboot del C128,
con VDC de 80 columnas y CPU en modo de 2 MHz. El sistema conservará los
servicios del KERNAL del C128 y dejará disponible la RAM bajo la ROM de BASIC
7.0. `EXIT` comprobará si hay aplicaciones suspendidas y pedirá confirmación
si las hay antes de indicar que se puede apagar el equipo manualmente.

`M:` será un volumen privado accesible desde CMD-OS y sus aplicaciones CMDP.
Usará un REU inicialmente y podrá incorporar NeoRAM Remix. Compartirá espacio
entre archivos normales y archivos `.swp` para suspender aplicaciones de
Bank 1. El disco virtual y el menú de sesiones estarán disponibles solo cuando
se detecte una expansión compatible y se monte el volumen. El menú permitirá
reanudar una aplicación o volver al intérprete para abrir otra.

Los [lineamientos de CMD-OS](doc/lineamientos-cmd-os.md) reúnen las decisiones de
diseño, el estado del prototipo y las instrucciones para retomar el trabajo en otra máquina.

## Funciones actuales

- Arranque desde una imagen `cmd-os.d64` en el dispositivo 8.
- Pantalla de texto de 80 columnas.
- Mensaje de arranque del bootloader.
- Prompt `CMD-OS A:>` estilo consola.
- Editor de línea mediante KERNAL/CC65.
- `DIR` para leer el directorio de la unidad activa.
- `EXIT` para regresar al BASIC 7.0.
- `RUN nombre.cmd` para cargar una aplicación externa. También podés escribir solo `nombre` o `nombre.cmd` si coincide con un CMDP de la unidad activa.
- `run mem` para ver las direcciones inicial y final de cada segmento de CMD-OS en el banco 0, junto con las reservas para aplicaciones y pilas.
- `run edit` para editar archivos de texto secuenciales de hasta 34 KiB en el banco 1. F7 abre los menús Archivo y Edición. Usa las flechas para navegar, Enter para elegir y Esc para cerrar el menú. F1 abre, F3 guarda, F5 guarda con otro nombre y F2 y F4 llevan al comienzo y al final. **Archivo → Nuevo** abre un documento en blanco y **Archivo → Salir** vuelve al intérprete.
- En los diálogos de archivo del editor, Enter con el nombre vacío lista los archivos SEQ de la unidad activa. Esc cancela tanto el diálogo como el selector.
- Selección `A:` = dispositivo 8, `B:` = 9, `C:` = 10 y `D:` = 11.
- Reserva de `M:` para el futuro disco virtual en memoria.

## Prototipo de aplicaciones externas

El sistema conserva el kernel y el intérprete en memoria. `RUN` carga programas
con formato `CMDP` en la zona temporal del banco 0, los ejecuta sin el stub de
BASIC y devuelve el control al intérprete cuando terminan.

`hello.cmd` y `mem.cmd` se ejecutan en el banco 0. `edit.cmd` se ejecuta en
el banco 1 y usa la API del kernel para dibujar, leer teclas y acceder a la
unidad activa. Los archivos del editor se guardan como `SEQ` en PETSCII,
con retorno de carro al final de línea.

## Compilar y ejecutar

Desde PowerShell:

```powershell
.\build.ps1
.\run-vice.ps1
```

La imagen resultante queda en `build\cmd-os.d64`.
`build.ps1` genera `build\boot.map` con el mapa de enlace que usa `mem.cmd`.
Si CC65 o VICE están instalados en otras rutas, indícalas con `-Cc65Bin` y `-ViceBin`.
