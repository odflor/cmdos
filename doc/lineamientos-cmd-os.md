# Lineamientos de CMD-OS

Documento de continuidad del proyecto. Integra los lineamientos del repositorio con las decisiones de diseño de las conversaciones de trabajo. El intérprete se identifica como **CMD-OS 0.2.0**. Las secciones de arquitectura describen el diseño acordado y los contratos del prototipo se documentan con el código como referencia. El orden de evolución al final es una propuesta.

## Diseño acordado para la próxima etapa

### Arranque autónomo y disco D71

- CMD-OS arrancará en modo nativo desde `build/cmd-os.d71`, la imagen de referencia para una 1571 en el dispositivo 8. El sector de autoboot de la pista 1, sector 0, cargará `BOOT` mediante el KERNAL del C128 y saltará al punto de entrada de CMD-OS. El runtime inicializará las pilas y la BSS sin ejecutar órdenes de BASIC 7.0 ni usar un stub BASIC.
- La MMU dejará visible la RAM en `$4000–$BFFF`, bajo la ROM de BASIC, y conservará el KERNAL, el editor de pantalla y la E/S del C128. El mapa de enlace determinará el espacio disponible, porque los CMDP y las pilas ya ocupan parte de esa ventana. Se conservarán inicialmente las direcciones de carga de los CMDP.
- CMD-OS seleccionará el VDC de 80 columnas y activará el modo de 2 MHz mediante `$D030` durante el arranque. Las aplicaciones seguirán regresando al prompt.
- `EXIT` comprobará si hay sesiones suspendidas antes de indicar que se puede apagar el equipo. Si las hay, indicará cuáles quedaron guardadas de forma persistente y cuáles se perderán al apagar, y pedirá confirmación. Si el usuario cancela, volverá al prompt. Si confirma o si no hay sesiones suspendidas, mostrará «Ya podés apagar el equipo». El apagado será manual.
- D71 da la capacidad de doble cara de la 1571. La velocidad de transferencia depende de la unidad y del protocolo, así que se medirá por separado la carga de `BOOT` y la lectura desde `RUN` y `EDIT` en las unidades que admitan transferencia rápida. Se verificará el autoboot en VICE con una 1571 y en los equipos que se declaren compatibles.

### Volumen privado M: y sesiones de Bank 1

- `M:` será un volumen privado de CMD-OS. El kernel administrará la expansión y las aplicaciones CMDP usarán sus servicios de archivos. El volumen quedará fuera de la interfaz de dispositivos del KERNAL del C128. El primer controlador será para REU mediante DMA y NeoRAM Remix podrá usar el mismo formato con un controlador de copia paginada. El disco virtual y la suspensión de aplicaciones estarán disponibles solo si se detecta una expansión compatible y se monta el volumen. Sin expansión compatible, `EXIT` podrá realizar la comprobación de apagado sin consultar archivos de intercambio.
- Los archivos normales y los archivos de intercambio `.swp` compartirán el espacio del volumen. `DIR` podrá mostrar ambos y el gestor de sesiones controlará la creación, restauración y borrado de los `.swp`. CMD-OS validará el formato antes de montar el volumen y requerirá una orden explícita para formatearlo.
- Si el contenido de la expansión se conserva o se restaura entre apagados, CMD-OS validará los `.swp` y su compatibilidad con la versión del sistema. Si encuentra sesiones recuperables, mostrará un aviso y seguirá hasta el prompt. La restauración se hará cuando el usuario elija una sesión mediante Alt+Help.
- Alt+Help, pendiente de comprobar en el C128 y en VICE, abrirá el menú desde el intérprete o desde un CMDP de Bank 1 en un punto seguro del servicio de teclado. Desde el intérprete se mostrarán las sesiones suspendidas, incluidas las persistentes recuperables. Desde un CMDP se mostrarán el intérprete y las demás sesiones suspendidas. El contexto activo queda fuera de la lista. Esc cierra el menú y devuelve el control al contexto activo sin guardar ni cargar un `.swp`. El intercambio empieza al elegir otro destino. El estado se guardará antes de reutilizar su memoria. Si falta espacio, se podrá volver a la aplicación activa.
- Cada sesión guardará las regiones de Bank 1 que usa la aplicación y el contexto para reanudarla. Para la primera prueba con `EDIT`, se propone copiar `$1000–$BFFF` (44 KiB), que incluye código, datos, BSS, pila de C y documento, y preservar la página cero de CC65, la pila del procesador y el mapa de memoria. El `.swp` prescindirá de la memoria VDC. Al reanudarse, `EDIT` invalidará su caché de pantalla y repintará la vista completa con el cursor y el desplazamiento guardados. El intérprete conservará su propia continuación y su unidad activa.
- En la primera versión, el punto de suspensión requerirá que la aplicación haya cerrado sus canales de disco. Si hay una operación en curso, la suspensión podrá esperar a que termine y se cierren los canales.

## Plataforma y arranque del prototipo actual

- Desarrollar para Commodore 128 en modo nativo, con CC65 (`-t c128`), C para el intérprete y ca65 para el arranque de aplicaciones. La consola usa el VDC de 80 columnas y las rutinas de pantalla, teclado y disco de CC65/KERNAL.
- El artefacto principal es `build/cmd-os.d64`, montado en el dispositivo 8. `build.ps1` compila y empaqueta los programas y escribe `boot`, `hello.cmd`, `mem.cmd` y `edit.cmd` con `c1541`. `tools/make-boot-sector.ps1` instala el sector de autoboot en pista 1, sector 0. El sector lleva la firma `CBM` y transfiere el control a BASIC con `RUN"BOOT"` mediante la entrada ROM `$AFA5`. Preservar este autoboot y el stub de BASIC de `boot.prg`.
- `run-vice.ps1` monta la imagen con `x128`. Los scripts apuntan a rutas de CC65 y VICE en `D:`. `boot.cmd` es otro lanzador de VICE con rutas absolutas y un disco adicional en el dispositivo 9.

## Consola y discos

| Entrada | Contrato actual |
| --- | --- |
| `A:`, `B:`, `C:`, `D:` | Seleccionan los dispositivos IEC 8, 9, 10 y 11. La consola inicia en `CMD-OS A:>`. |
| `DIR` | Lee la unidad activa y muestra cabecera, archivos, tipos, tamaños y bloques libres. |
| `RUN nombre[.cmd]` | Abre el archivo en la unidad activa y agrega `.cmd` si el nombre carece de punto. El argumento conserva sus caracteres originales. |
| `nombre[.cmd]` | Si no es un comando interno, busca una coincidencia exacta con el nombre de un archivo `.cmd` en la unidad activa y utiliza el mismo cargador que `RUN`. |
| `HELP`, `VER` | Muestran los comandos y la versión. |
| `EXIT` | Devuelve el control a BASIC 7.0. |
| `M:` | Informa que la unidad está reservada para un futuro disco virtual. |

La comparación de comandos ignora diferencias entre las letras ASCII minúsculas y mayúsculas. Mantener el flujo de vuelta al prompt tras cada operación, incluidos los errores de disco o formato. Conservar el tratamiento PETSCII al imprimir nombres del disco y `\r\n` en las salidas de la consola.

## Arquitectura de memoria y servicios del kernel

El diseño acordado mantiene el **kernel de CMD-OS y el intérprete residentes**. El kernel tiene su base en Bank 0 y una puerta de entrada en RAM común visible desde ambos bancos. El intérprete usa los mismos servicios públicos que las aplicaciones y queda suspendido mientras se ejecuta un programa. Al finalizar, retoma la ejecución en memoria, sin recargarse desde disco. Esto es especialmente importante para encadenar comandos externos en archivos por lotes.

Solo se ejecuta **una aplicación a la vez**, sin multitarea ni aplicaciones residentes. Una aplicación pequeña se enlaza para la ventana temporal de Bank 0, junto al intérprete. Una aplicación grande se prepara para Bank 1 y puede usar la ventana temporal de Bank 0 como memoria auxiliar para textos, gráficos, música u otros datos. Los comandos por lotes y las aplicaciones interactivas pueden pertenecer a cualquiera de las dos clases. El banco depende del mapa para el que se enlazó el binario y de sus requisitos de memoria.

La API común debe empezar con servicios de consola, teclado y disco. La puerta de entrada debe poder seleccionar el mapa apropiado para el servicio y restaurar el de la aplicación antes de devolverle el control. Queda por fijar qué parte del kernel cabrá en RAM común, la ubicación y versión de la ABI, las convenciones de llamada y la preservación de página cero, pilas, MMU e interrupciones.

La ventana temporal de Bank 0 se presta por completo al programa activo. Puede contener una aplicación pequeña o los recursos de una grande. Al regresar al intérprete se invalida toda esa asignación y la siguiente ejecución puede reutilizarla. Para acceder a recursos desde Bank 1, se propone usar identificadores y copias por bloques a través del kernel. Un puntero C corriente expresa una dirección del mapa activo. El servicio que cruza bancos necesita conocer también el banco del búfer.

El prototipo integra el intérprete, el cargador y `src/kernel/tui.c` en el programa de arranque. La primera API de Bank 1 ya ofrece pantalla, teclado y archivos secuenciales. El préstamo de recursos de Bank 0 a aplicaciones de Bank 1 sigue pendiente.

### Bank 1 y la API TUI

| Rango | Uso |
| --- | --- |
| `$0000–$0FFF` | RAM común temporal durante carga y ejecución. El puente empieza en `$0E00` y el buzón en `$0F70`. CMD-OS conserva y restaura la zona de sprites que usa el puente. |
| `$1000–$33FF` | Hasta 9 KiB para código, datos y BSS del CMDP Bank 1. |
| `$3400–$37FF` | Pila de C del CMDP Bank 1. |
| `$3800–$BFFF` | Búfer de texto de `EDIT`, con 34 KiB para el documento. |

La ABI 2 usa un buzón común con operaciones y un búfer de 128 bytes. El puente preserva la página cero de CC65 y cambia al mapa de Bank 0 antes de ejecutar un servicio. Los servicios usan la pila de C del intérprete. Al volver, el puente restaura el contexto del CMDP y su mapa. La puerta ofrece limpiar, pintar un tramo de texto, posicionar el cursor, leer una tecla, dibujar un diálogo y abrir, transferir y cerrar archivos de la unidad activa. Las IRQ se habilitan dentro del servicio de Bank 0.

La paleta estándar vive en `src/kernel/tui.h`. Usa el cian sobre negro de BASIC 7.0 en 80 columnas y reserva el amarillo para los cuadros de diálogo y menús. El servicio TUI dibuja los bordes con caracteres de línea simple. `run edit` abre el editor VDC. F7 despliega los menús Archivo y Edición, que se recorren con las flechas y se activan con Enter. Esc o F7 cierra el menú. F1 abre, F3 guarda y F5 guarda con otro nombre. Para salir, elegir **Archivo → Salir**. F2 y F4 llevan al comienzo y al final. La barra de estado muestra `Archivo:` seguido del nombre actual.

### Regla general de los diálogos de archivo

Hasta nuevo aviso, en todo CMD-OS, Enter con el nombre vacío abre el selector de la unidad activa, filtrado al tipo que corresponda a la operación. Las flechas arriba y abajo recorren la lista y Enter elige el archivo. Esc cancela desde el campo de nombre, el selector o un diálogo de confirmación. Un nombre ya escrito o sugerido se acepta con Enter. El editor utiliza el servicio común `TUI_PICK_FILE` con el filtro SEQ. Al elegir un archivo distinto del actual como destino de guardado, el editor pide confirmar su reemplazo.

El editor lee archivos SEQ de la unidad activa, acepta finales CR, LF o CRLF y guarda en PETSCII con final CR. El límite de 34 KiB corresponde al documento una vez normalizados los saltos de línea. El diálogo de guardado propone el nombre actual. Para reemplazarlo, el editor escribe y cierra `cmdos-edit.tmp`, conserva el original como `cmdos-edit.bak` durante el cambio de nombre y limpia el respaldo después de confirmar la operación. La unidad debe tener espacio para el temporal. El editor redibuja solo los caracteres que cambiaron y, en las ediciones que conservan el ajuste de línea, evita recorrer todo el documento visible.

## Aplicaciones externas

### Filosofía de uso de la memoria

`src/cmdos.c` conserva el estado del intérprete, incluida la unidad activa, mientras `RUN` presta a una aplicación una zona temporal de RAM. El cargador lee primero la cabecera, comprueba los requisitos y copia el ejecutable directamente a su dirección de enlace. Cierra los canales antes de llamar a la entrada del programa. La llamada es síncrona y el intérprete sigue desde el mismo punto cuando la aplicación retorna. Cada binario se enlaza para su ubicación y el formato actual carece de un mecanismo de reubicación. La bandera declara compatibilidad y no modifica las direcciones del binario.

Para los comandos externos por lotes, el intérprete deberá pasar argumentos y recibir un código de salida al recuperar el control. Así podrá seguir con la próxima línea y, más adelante, tomar decisiones según el resultado. El contrato de argumentos, la sintaxis de lotes y la exposición del código de salida quedan pendientes. También queda como ampliación la posibilidad de elegir un intérprete al arrancar mediante un archivo de configuración.

`src/apps/hello.c` valida el flujo mínimo. Usa `cputs` y `cgetc` sobre la consola que el intérprete ya abrió, espera una tecla y sale de `main`. El programa trae rutinas de CC65 enlazadas en su propio binario. El servicio compartido que demuestra es la consola del C128. Las llamadas a la futura API del kernel de CMD-OS quedan por implementar. La salida de `HELLO` permanece visible al volver al prompt.

El runtime de la aplicación (`src/runtime/cmdos-app-crt0.s`) selecciona el mapa CC65 de Bank 0, guarda la página cero usada por CC65 e inicializa la BSS y su propia pila de C antes de llamar a `main`. Al salir, restaura esa porción de página cero (incluido el puntero a la pila de C del intérprete), el puntero de la pila del procesador, `INIT_STATUS` y el mapa MMU. Escribe el valor de salida en `STATUS` y vuelve con `RTS`. El intérprete no consume ese valor. Es un contrato de ejecución cooperativa de código nativo. La página cero se reutiliza por turnos, la pila de C de la aplicación tiene su propio espacio y la pila del procesador conserva la cadena de llamadas.

### Mapa efectivo y límites de Bank 0

| Rango | Uso o reserva actual |
| --- | --- |
| `$0002–$001B` | Reserva del enlazador para la página cero de CC65. El runtime guarda y restaura los bytes de trabajo de CC65. |
| `$1C01–$3D67` | Desde el stub BASIC hasta el final de la BSS del intérprete en la compilación revisada. La dirección final depende del build. |
| Fin de CMD-OS hasta `$4FFF` | Margen para el crecimiento del kernel y del intérprete. |
| `$5000–$AFFF` | Área de enlace de aplicaciones de 24 KiB para código, datos y reservas en RAM, incluidas INIT y BSS. |
| `$B000–$B7FF` | Reserva para la pila de C de la aplicación, que crece hacia abajo desde `$B800`. |
| `$B800–$BFFF` | Reserva de la pila de C del intérprete con la configuración estándar instalada de CC65. |

El enlazador, el empaquetador y el cargador aplican `$B000` como límite exclusivo del área CMDP de Bank 0. La cabecera declara los bytes en disco. El enlazador comprueba también `INIT`, `BSS` y el límite inferior de la pila de C de la aplicación. El runtime inicia esa pila en `$B800`, en RAM visible con el mapa CC65. El build verifica que los segmentos residentes terminen antes de `$5000`. `run mem` muestra los segmentos de `boot.map`, las reservas y el margen para ampliar CMD-OS. La imagen incorpora `hello.cmd` y `mem.cmd` recompilados para la nueva dirección de carga.

En Bank 1 se mantiene la misma idea de área temporal. Lo que cambia es el banco en que viven el código y los datos de la aplicación. El intercambio con el kernel debe especificar a qué banco pertenece cada búfer, ya que una misma dirección puede referirse a RAM distinta al cambiar el mapa. La reserva para `M:` deberá formar parte del mismo presupuesto de memoria para preservar su contenido entre ejecuciones.

### Cabecera CMDP v1

La cabecera ocupa 16 bytes, seguida por el ejecutable crudo sin dirección PRG ni stub BASIC. Los campos de dos bytes se guardan en orden little-endian.

Comparar la firma como bytes fijos. Los literales de caracteres de CC65 para C128 pueden tener valores PETSCII distintos de los bytes binarios de la cabecera. Mantener en minúsculas los nombres grabados en la imagen y conservar el nombre recibido por `RUN`.

| Offset hexadecimal | Campo | Comportamiento actual |
| --- | --- | --- |
| `00–03` | Firma | Bytes `43 4D 44 50` (`CMDP`). |
| `04` | Versión del formato | `01`. |
| `05` | Bancos | `01` para Bank 0, `02` para Bank 1 y `03` para ambos. `03` se ejecuta en Bank 0. |
| `06–07` | Dirección de carga | `$5000` para Bank 0 y `$1000` para Bank 1. |
| `08–09` | Desplazamiento de entrada | Relativo a la dirección de carga. El empaquetador escribe cero. El cargador exige que esté dentro del contenido. |
| `0A–0B` | Tamaño en bytes | Se exige una lectura completa y un máximo de `$6000` bytes para Bank 0 o `$2400` para Bank 1. |
| `0C` | Versión de API solicitada | Bank 0 acepta `0` y `1`. El cargador exige `2` para Bank 1. |
| `0D–0F` | Reservados | El empaquetador escribe ceros. |

El enlazador reserva por defecto `$0800` bytes para la pila de C bajo `$B800`. Los segmentos de la aplicación, incluidas BSS e INIT, deben caber debajo de `$B000`.

## Material de referencia del repositorio

- `lib/` contiene demostraciones independientes en C y sus `.prg` de VDC. `lorem128` y `vdc_real_40_columns` ensayan 40 columnas con el VDC. `spotlight128` desplaza una vista de 40 columnas. `vdc_blink`, `vdc_blink2` y `vdc_random_art` trabajan con atributos y VRAM. `mandala128` genera patrones de texto. Los `.bat` disponibles compilan ejemplos individuales.
- [`lib/example.c`](../lib/example.c) es una demostración en C de la consola VDC de 80 columnas. Se puede compilar por separado con `cl65 -t c128 -O -o example-vdc.prg lib/example.c`.
- [`doc/programa.prg`](programa.prg) y [`doc/KeystoneKapers.prg`](KeystoneKapers.prg) son binarios conservados como material de referencia. Ambos se trasladaron desde la raíz con su contenido intacto. El build de CMD-OS trabaja con los fuentes de `src/`, y estos binarios quedan fuera de la imagen D64.
- `crazy-lander/` trae su propio fuente, documentación, compilador por lote y `.prg` para VIC-II. Se compila por separado.
- `doc/c128 bootsect.7z` contiene `bootsect.128`, de 256 bytes. El proceso de compilación genera su propio sector.
- `LICENSE` contiene la licencia MIT del repositorio. `.gitignore` excluye `build/` y los `.o`. Los scripts de Git trabajan sobre `main`. `bajar-cambios.ps1` usa `pull --rebase`; `subir-cambios.ps1` agrega todo el árbol antes de crear un commit y hacer push.

## Historial de trabajo

1. Los commits `3c6e15a` y `aa08ad1` establecieron el arranque desde D64, la consola VDC de 80 columnas y la ejecución de aplicaciones externas CMDP. `92c5b9b` consolidó los fuentes del repositorio.
2. Se confirmó la arquitectura del proyecto. El kernel y el intérprete quedan residentes en Bank 0. Las aplicaciones pequeñas usan una reserva temporal en Bank 0 y las grandes usarán Bank 1. Ambos bancos deben poder llamar a la misma API del kernel. `RUN` ejecuta una aplicación por vez y retoma el intérprete al terminar.
3. Se detectó un solapamiento entre la ventana CMDP inicial y la pila de C del intérprete. Al probar `mem.cmd` en VICE también se encontró que la primera reserva de pila de la aplicación quedaba bajo ROM con el mapa activo de CC65. El mapa actual coloca las dos pilas en RAM visible, en reservas distintas.
4. Se incorporó `mem.cmd`, generado a partir del mapa de enlace de `boot.prg`. Muestra los rangos inclusivos, el tamaño en KiB de cada segmento, el total asignado y la RAM libre sin asignar dentro del mapa de CMD-OS.
5. Se corrigió la cabecera de `DIR` para mostrar el nombre, el ID y el tipo DOS. En VICE se verificó `"cmd-os" co 2a`, junto con el listado de archivos.
6. Se amplió el área CMDP a 24 KiB (`$5000–$AFFF`). Se recompilaron `hello.cmd` y `mem.cmd` para la nueva dirección. La pila CMDP quedó en `$B000–$B7FF` y la del intérprete en `$B800–$BFFF`. En VICE ambas aplicaciones se ejecutaron y volvieron al prompt. El último listado revisado mostró 36,32 KiB asignados y 4,70 KiB libres para ampliar CMD-OS (`$3D32–$4FFF`). Estas cifras se recalculan en cada compilación.
7. Se añadió `edit.cmd` como primer programa de Bank 1, junto con el puente MMU y los servicios TUI residentes de ABI 2. En VICE se abrió un SEQ de 33.792 bytes, se añadió un carácter al final y se verificaron los 33.793 bytes de la copia guardada. Al salir, el intérprete retomó el control y `dir` mostró ambos archivos.
8. Se adoptó el color de texto del arranque de BASIC 7.0 en el VDC, se agregaron bordes simples y menús desplegables, y se movió el nombre del documento a la barra de estado. El guardado propone el nombre actual y usa un temporal y un respaldo al reemplazar un archivo. Se redujo el redibujado al editar.
9. Se amplió el área de edición a las 80 columnas del VDC y a 21 filas visibles, desde la primera fila debajo del menú. Las líneas largas se ajustan visualmente a la siguiente fila sin agregar un salto al archivo.
10. F7 abre el menú. **Archivo → Nuevo** vuelve a un documento en blanco y sin nombre. **Archivo → Salir** vuelve al intérprete. Ambas opciones piden confirmación si hay cambios sin guardar.
11. El intérprete conserva `RUN` y permite iniciar un CMDP escribiendo solo su nombre, con o sin `.cmd`. Los comandos internos tienen prioridad.
12. Se incorporó un selector de archivos reutilizable en el kernel. El editor lista los SEQ de la unidad activa cuando se presiona Enter con el nombre vacío. Se adoptó esta regla para los futuros diálogos de archivo de CMD-OS y se actualizó el resumen ejecutivo del README.

## Retomar el proyecto en otra máquina

Actualizar este documento y `README.md` únicamente cuando el usuario solicite explícitamente la actualización del archivo correspondiente. Las novedades de varias sesiones se pueden documentar juntas. Cuando se solicite actualizar los lineamientos, registrar las decisiones, el estado comprobado y el siguiente paso.

Usar este documento versionado para trasladar el contexto entre máquinas. En la otra máquina, clonar el repositorio o actualizar la copia existente con `git pull --ff-only origin main` desde `main`. Abrir esa carpeta en Codex y empezar con este mensaje:

> Lee `doc/lineamientos-cmd-os.md` y usa su historial como contexto. Revisa el estado del repositorio y retomemos CMD-OS desde allí.

La imagen D64 y los demás resultados compilados se regeneran con `build.ps1`, porque `build/` está excluido de Git. En la máquina donde se hicieron estas pruebas se usó:

```powershell
.\build.ps1 -Cc65Bin C:\cc65\bin -ViceBin C:\GTK3VICE-3.7-win32\bin
& 'C:\GTK3VICE-3.7-win32\bin\x128.exe' -80col -8 .\build\cmd-os.d64
```

Adaptar las rutas a la instalación de cada equipo. La comprobación básica usa `dir`, `run hello`, `run mem` y `run edit`. Escribir en minúsculas los nombres de los archivos tal como figuran en el disco. Quedan pendientes el préstamo de recursos, los argumentos y códigos de salida de aplicaciones, los archivos por lotes y el disco virtual `M:`.

## Orden de evolución propuesto

1. Implementar el arranque autónomo desde D71 y validar en `x128` con una 1571 en el dispositivo 8. Comprobar el prompt en VDC a 2 MHz, `ver`, `dir`, `run hello`, `run mem`, la apertura y el guardado de un SEQ con `run edit`, y el regreso al prompt.
2. Mantener las reservas del intérprete y de las aplicaciones separadas. Probar el puente y el editor también en hardware real.
3. Adaptar las aplicaciones de Bank 0 a los servicios comunes y acordar una ABI para compartir recursos entre bancos.
4. Probar el uso de la ventana temporal de Bank 0 para recursos de una aplicación Bank 1 y la reutilización de esa misma ventana por una aplicación Bank 0 después del retorno.
5. Pasar argumentos y devolver códigos de salida. Validar una secuencia de comandos externos que conserve el estado del intérprete entre ejecuciones.
6. Implementar el volumen privado `M:` sobre REU e incorporarlo a `DIR`, `RUN` y los servicios de archivos de CMD-OS. Probar la suspensión y reanudación alternada de dos sesiones de `EDIT` con documentos sin guardar. Agregar el controlador opcional para NeoRAM Remix sobre el mismo formato.
