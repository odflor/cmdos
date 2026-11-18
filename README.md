# CMD-OS

A DOS-like operating system for the C128

Primer prototipo de un entorno de línea de comandos nativo para Commodore 128.

## Funciones actuales

- Arranque desde una imagen `cmd-os.d64` en el dispositivo 8.
- Pantalla de texto de 80 columnas.
- Mensaje de arranque del bootloader.
- Prompt `CMD-OS A:>` estilo consola.
- Editor de línea mediante KERNAL/CC65.
- `DIR` para leer el directorio de la unidad activa.
- `EXIT` para regresar al BASIC 7.0.
- `RUN nombre.cmd` para cargar una aplicación externa CMD-OS.
- `run mem` para ver las direcciones inicial y final de cada segmento de CMD-OS en el banco 0, junto con las reservas para aplicaciones y pilas.
- Selección `A:` = dispositivo 8, `B:` = 9, `C:` = 10 y `D:` = 11.
- Reserva de `M:` para el futuro disco virtual en memoria.

## Prototipo de aplicaciones externas

El sistema conserva el kernel y el intérprete en memoria. `RUN` carga programas
con formato `CMDP` en la zona temporal del banco 0, los ejecuta sin el stub de
BASIC y devuelve el control al intérprete cuando terminan.

La primera aplicación de prueba es `hello.cmd`. Su cabecera ya incluye una
bandera de banco: el cargador acepta `BANK0` en esta versión y deja preparado el
formato para `BANK1`, que se implementará junto con el cambio de MMU y la API
común del kernel.

## Compilar y ejecutar

Desde PowerShell:

```powershell
.\build.ps1
.\run-vice.ps1
```

La imagen resultante queda en `build\cmd-os.d64`.
`build.ps1` genera `build\boot.map` con el mapa de enlace que usa `mem.cmd`.
Si CC65 o VICE están instalados en otras rutas, indícalas con `-Cc65Bin` y `-ViceBin`.
