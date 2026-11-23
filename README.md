# Command-OS (cmd-os)

## 1. Resumen ejecutivo

Command-OS es un entorno de trabajo al estilo DOS para el Commodore 128 en modo nativo. Arranca desde una imagen D71 en la unidad 1571 del dispositivo 8, usa la pantalla VDC de 80 columnas y trabaja con la CPU a 2 MHz. Al iniciar, informa la memoria de video instalada y la presencia de una REU.

El proyecto reúne una consola de comandos, un editor de texto SEQ y servicios compartidos para aplicaciones nativas. Con una REU compatible de 16 MiB, también ofrece el disco en memoria `M:` y permite suspender aplicaciones de Bank 1 y retomarlas más tarde. Command-OS está en desarrollo. Los archivos por lotes y el soporte de NeoRAM Remix quedan para una etapa posterior.

## 2. Ayuda para el usuario final

En VICE, montá `build/cmd-os.d71` en la 1571 del dispositivo 8 y seleccioná la pantalla de 80 columnas. El lanzador del proyecto hace esa configuración automáticamente. Cuando aparezca el prompt `CMD-OS A:>`, escribí `HELP` o pulsá la tecla física **HELP**. Conservá la imagen de arranque en el dispositivo 8 para que el sistema pueda cargar la guía y el comando `COPY`.

| Comando | Uso |
| --- | --- |
| `A:`, `B:`, `C:`, `D:` | Seleccionan los dispositivos IEC 8, 9, 10 y 11. |
| `DIR` | Lista los archivos de la unidad activa, sus tipos, tamaños y espacio disponible. |
| `RUN nombre [argumentos]` | Ejecuta un programa de la unidad activa. También podés escribir directamente su nombre y omitir la extensión `.cmd`. |
| `COPY origen destino` | Copia archivos PRG, SEQ o USR. Indicá las dos unidades, por ejemplo `COPY A:edit.cmd M:edit.cmd`. Si el destino existe, la copia se rechaza. |
| `DEL nombre` | Borra un archivo de la unidad activa. |
| `FORMAT M:`, `M:` | Inicializan el disco REU tras confirmar y seleccionan el volumen montado, respectivamente. |
| `STATUS`, `MEM`, `VER` | Muestran el último código de salida de un CMDP, el mapa de memoria y la versión del sistema, respectivamente. |
| `EXIT` | Indica cuándo se puede apagar. Si hay sesiones suspendidas, las lista y pide confirmación. |

Los nombres de archivo distinguen mayúsculas y minúsculas. Admiten hasta 16 caracteres PETSCII con letras, dígitos, guiones y guiones bajos, más un punto opcional para separar la extensión. Escribilos sin espacios. Los archivos que genera Command-OS usan minúsculas.

### Editar documentos

`EDIT` abre un documento vacío. `EDIT notas.seq` abre el SEQ indicado en la unidad activa. Si el archivo aún no existe, inicia un documento vacío y propone ese nombre al guardar. El editor admite documentos de hasta 34 KiB en IEC o en `M:`.

- **F1** abre un archivo. En el diálogo de apertura, Enter con el nombre vacío muestra el selector de archivos SEQ.
- **F3** guarda. **F7** abre los menús. Elegí **Archivo → Guardar como** para usar otro nombre y **Archivo → Salir** para volver a la consola.
- **F2** y **F4** llevan al inicio y al final del documento.

### Usar M: y cambiar de sesión

`M:` tiene 16 ranuras de 64 KiB y admite hasta 65.535 bytes por archivo. `DIR` muestra el tamaño y el tipo IEC de cada entrada, además del espacio asignado y libre por ranuras. Los documentos y las sesiones suspendidas comparten esas ranuras.

Con `M:` montado, pulsá **Alt+Tab** desde el editor o la consola. En VICE, usá **Ctrl+Y** si el sistema anfitrión captura el atajo. Desde el editor podés elegir el intérprete u otra sesión suspendida. Desde la consola podés reanudar una sesión o elegir **Descartar** y confirmar su eliminación. **Esc** vuelve a la pantalla activa. El cambio de sesión requiere que los canales de disco estén cerrados. Si la REU conserva su contenido al reiniciar, Command-OS anuncia las sesiones recuperables.

## 3. Estructura interna para desarrolladores

El intérprete y el kernel residen en **Bank 0**. El intérprete procesa la línea de comandos, administra la unidad activa y carga los ejecutables CMDP. Los módulos `src/kernel/cmdosk-*` implementan el despacho de servicios, la interfaz de texto, el teclado, los menús, los archivos y las sesiones. `src/apps/` contiene las aplicaciones y `src/runtime/` contiene el código de arranque.

- **Carga de aplicaciones.** Los CMDP de Bank 0 se enlazan en `$7400–$B3FF`, con 16 KiB para la aplicación y 1 KiB de pila de C en `$B400–$B7FF`. `edit.cmd` se enlaza en `$1000` de Bank 1 y usa `$3800–$BFFF` para el documento. Solo se ejecuta una aplicación a la vez.
- **ABI 1.** Los CMDP invocan servicios mediante la puerta en `$0E00` y el buzón en `$0F70`, con un búfer de 128 bytes. El puente conserva el contexto de Bank 1, cambia el mapa de la MMU para atender el servicio y restaura el mapa de la aplicación. El contrato en C está en [`src/kernel/cmdosk-abi.h`](src/kernel/cmdosk-abi.h).
- **Disco.** El kernel ofrece dos canales simultáneos compartidos entre IEC y la REU. Cada apertura devuelve un identificador para leer, escribir y cerrar. `EDIT` usa esos servicios mediante el buzón y `COPY` usa las mismas rutinas dentro del kernel.
- **Sesiones.** El kernel guarda los 44 KiB de Bank 1 que utiliza la aplicación y su contexto en un archivo `.swp` mediante DMA de la REU. Verifica la integridad antes de restaurar y libera la ranura tras reanudar con éxito.

Durante el desarrollo se mantiene una sola ABI. Recompilá el kernel y los CMDP juntos al cambiar el contrato y actualizá las copias de aplicaciones que hayas guardado en `M:`. El [documento de lineamientos](doc/lineamientos-cmd-os.md) detalla el mapa, los campos del buzón y el formato CMDP. La carpeta [`examples/`](examples/) reúne proyectos y material de referencia en código fuente, programas, imágenes de disco y archivos comprimidos.

## 4. Herramientas, configuración y compilación

Los scripts del repositorio se ejecutan en PowerShell sobre Windows. Para compilar y probar se usan estas herramientas.

1. Descargá y extraé **cc65** con sus bibliotecas para Commodore 128. La carpeta `bin` debe contener `cl65.exe`, que coordina la compilación, el ensamblado y el enlace. [Proyecto cc65](https://cc65.github.io/)
2. Descargá y extraé **VICE para Windows**. Su carpeta `bin` debe contener `c1541.exe`, que empaqueta la imagen D71, y `x128.exe`, que emula el Commodore 128. Los ejemplos de ruta siguientes usan la instalación de VICE 3.9. [Descargas oficiales de VICE](https://vice-emu.sourceforge.io/#download)
3. Abrí PowerShell en la raíz del repositorio y ajustá las dos rutas a tu instalación. Los scripts reciben las carpetas `bin` y no requieren agregarlas a `PATH`.

```powershell
$cc65Bin = 'D:\Program Files (x86)\cc65\bin'
$viceBin = 'D:\Program Files (x86)\GTK3VICE-3.9-win64\bin'

.\build.ps1 -Cc65Bin $cc65Bin -ViceBin $viceBin
.\run-vice.ps1 -ViceBin $viceBin
```

El build genera `build/cmd-os.d71` con el sector de arranque, `boot` y los CMDP `hello.cmd`, `mem.cmd`, `copy.cmd`, `help.cmd` y `edit.cmd`. También deja los binarios intermedios y el mapa de enlace `build/boot.map`. El lanzador abre la imagen principal en la 1571 del dispositivo 8 y configura el emulador con 80 columnas, 16 KiB de VRAM y 16 MiB de REU. Para probar el arranque, ejecutá `DIR`, `HELP`, `MEM` y `EDIT` en la consola.

## 5. Filosofía de Command-OS

Command-OS parte de una idea sencilla. Queremos que el Commodore 128 sea cómodo de usar para trabajar con archivos, escribir texto y probar aplicaciones propias. La consola mantiene las tareas habituales cerca y el kernel resuelve una sola vez las operaciones que varias aplicaciones necesitan, como abrir un archivo o mostrar un menú.

La memoria disponible, las unidades IEC y el VDC forman parte del diseño cotidiano. Una expansión como la REU agrega posibilidades concretas, como retomar un documento sin guardar, y la consola sigue siendo utilizable en un C128 sin esa expansión. Si querés entender una rutina o probar otra forma de resolverla, podés empezar por los ejemplos y seguir el código hasta el servicio que usa.
