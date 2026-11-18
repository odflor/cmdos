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
- Selección `A:` = dispositivo 8, `B:` = 9, `C:` = 10 y `D:` = 11.
- Reserva de `M:` para el futuro disco virtual en memoria.

## Compilar y ejecutar

Desde PowerShell:

```powershell
.\build.ps1
.\run-vice.ps1
```

La imagen resultante queda en `build\cmd-os.d64`.
