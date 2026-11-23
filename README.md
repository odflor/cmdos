# Command-OS (cmd-os)

CMD-OS es un entorno al estilo DOS para el Commodore 128 en modo nativo.
Arranca desde una D71 en la 1571 del dispositivo 8 y usa el VDC de 80 columnas
con la CPU a 2 MHz. El arranque informa la VRAM instalada y la REU detectada.

## Consola y aplicaciones

- `A:` a `D:` seleccionan los dispositivos IEC 8 a 11. `DIR` lista la unidad activa.
- `RUN nombre [argumentos]` o `nombre [argumentos]` ejecuta un CMDP. `STATUS` muestra el último código de salida.
- `DEL nombre` borra en la unidad activa. `COPY origen destino` exige las dos unidades, por ejemplo `COPY A:edit.cmd M:edit.cmd`, y admite PRG, SEQ y USR sin reemplazar el destino.
- Los nombres distinguen mayúsculas y minúsculas. Se admiten letras, dígitos, guion y guion bajo, con un punto opcional antes de la extensión. CMD-OS genera nombres en minúsculas.
- `EDIT` abre y guarda documentos SEQ de hasta 34 KiB en IEC o en `M:`. `EDIT nombre` abre ese SEQ en la unidad activa o propone el nombre al guardar si el archivo todavía no existe. F1 abre, F3 guarda y F7 abre los menús. **Guardar como** está en el menú Archivo. Enter con el nombre vacío muestra un selector filtrado por tipo.
- Escribí `HELP` o pulsá la tecla HELP en el intérprete para abrir la guía de comandos desde la unidad 8.
- `MEM` informa el mapa de ambos bancos, las reservas, el margen del kernel y la detección de REU. `EXIT` lista las sesiones suspendidas y pide confirmación antes de apagar.

## Disco privado M:

Con una REU de 16 MiB compatible, `FORMAT M:` inicializa el volumen tras pedir
confirmación. `M:` lo selecciona y permite usar `DIR`, `RUN`, `DEL`, `COPY`
y `EDIT`. El formato actual tiene 16 ranuras de 64 KiB, con un límite de
65.535 bytes por archivo. `DIR` informa los tamaños, los tipos IEC y el espacio
asignado y libre por ranuras. El listado deja una línea en blanco después
de la cabecera y antes de los totales. `COPY` carga su aplicación desde la
unidad 8 y delega la transferencia al kernel.

## Suspender y reanudar aplicaciones

Con `M:` montado, pulsá `Alt+Tab` desde el intérprete o desde `EDIT`.
En VICE podés usar `Ctrl+Y` si el anfitrión captura `Alt+Tab`. Desde
`EDIT`, elegí **Interprete** u otra sesión suspendida. Desde el intérprete,
elegí una sesión para reanudarla. Esc vuelve a la pantalla activa. El
intercambio muestra su progreso y guarda cada sesión en un archivo `.swp`
de `M:`. Al reanudar `EDIT`, se recupera el documento con sus cambios sin
guardar y se repinta la pantalla. La opción **Descartar** libera una sesión
suspendida tras confirmar. Si la REU conserva su contenido al reiniciar,
CMD-OS anuncia las sesiones disponibles y permite reanudarlas con `Alt+Tab`.

La interfaz actual es ABI 1. Recompilá los CMDP de compilaciones anteriores y reemplazá las copias guardadas en `M:`.

## Memoria

El kernel y el intérprete residen en Bank 0. Los CMDP de ese banco tienen
16 KiB en `$7400–$B3FF` y necesitan estar enlazados para `$7400`. Reemplazá
las copias antiguas de `hello.cmd`, `mem.cmd` y `copy.cmd` guardadas en la REU.
`edit.cmd` sigue en `$1000` de Bank 1. CMD-OS reutiliza parte de la pantalla
VIC-II para respaldar el área común y parte de la RAM de color para respaldar
la configuración del teclado.

Quedan pendientes los lotes y el soporte de NeoRAM Remix. Consultá los
[lineamientos de CMD-OS](doc/lineamientos-cmd-os.md) para el mapa y los contratos
del prototipo.

## Compilar y ejecutar

```powershell
.\build.ps1
.\run-vice.ps1
```

El build genera `build\cmd-os.d71` y `build\boot.map`. Usá `-Cc65Bin` y
`-ViceBin` para indicar otras rutas de herramientas. `run-vice.ps1` monta la
imagen habitual en la 1571 y configura el VDC con 16 KB y la REU con 16 MiB.
