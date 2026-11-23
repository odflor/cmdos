# Lineamientos de CMD-OS

Documento de continuidad del proyecto. Integra los lineamientos del repositorio con las decisiones de diseño de las conversaciones de trabajo. El intérprete se identifica como **CMD-OS 0.2.0**. Las secciones de arquitectura describen el diseño acordado y los contratos del prototipo se documentan con el código como referencia. El orden de evolución al final es una propuesta.

## Diseño acordado para la próxima etapa

### Pruebas pendientes de la 1571

Medir por separado la carga de BOOT y la lectura desde RUN y EDIT en unidades que admitan transferencia rápida.

### Volumen privado M: y sesiones de Bank 1

El kernel abre un menú centrado con `Alt+Tab` desde el intérprete o desde
el bucle principal de teclado de `EDIT`. `Ctrl+Y` sirve como alternativa en
VICE. El menú ofrece las sesiones suspendidas y, desde `EDIT`, la opción
**Interprete**. Esc vuelve al contexto activo. El intercambio comienza al
seleccionar un destino.

- `M:` es un volumen privado del kernel. La suspensión requiere una REU compatible y un volumen montado. Si falta el volumen, el kernel muestra el motivo en un diálogo.
- El intercambio muestra su progreso y usa DMA de la REU. Cada `.swp` ocupa una ranura de `M:`, que el gestor libera tras reanudar la sesión. Si falla el guardado, se puede volver a la aplicación activa.
- El archivo guarda los 44 KiB de `$1000–$BFFF` de Bank 1, la página cero de CC65, la pila y el contexto del procesador, el mapa MMU y la unidad activa. El gestor verifica la escritura y comprueba el CRC antes de restaurar. `EDIT` repinta su barra de menú, el documento y el cursor al reanudar.
- El cargador cierra su canal antes de transferir el control al CMDP. El gestor bloquea el cambio de sesión si hay un archivo abierto en `M:` o un canal de disco IEC en la tabla de archivos lógicos del KERNAL.
- `EXIT` lista las sesiones suspendidas y pide confirmar el apagado. N y Esc vuelven al intérprete. En el menú de sesiones, **Descartar** permite elegir una sesión y liberar su ranura después de confirmar.
- Si la REU conserva el volumen al reiniciar, el arranque anuncia las sesiones cuya cabecera coincide con la ABI y el tamaño de contexto. La reanudación comprueba además el CRC y el contexto antes de transferir el control. Quedan pendientes las pruebas en hardware real y el controlador para NeoRAM Remix.

## Plataforma y arranque del prototipo actual

- Desarrollar para Commodore 128 en modo nativo, con CC65 (`-t c128`), C para el intérprete y ca65 para el arranque de aplicaciones. La consola usa el VDC de 80 columnas y las rutinas de pantalla, teclado y disco de CC65/KERNAL.
- El artefacto principal es `build/cmd-os.d71` para la 1571 del dispositivo 8. `build.ps1` empaqueta `boot`, `hello.cmd`, `mem.cmd`, `copy.cmd`, `help.cmd` y `edit.cmd` con `c1541`. `tools/make-boot-sector.ps1` instala y reserva la pista 1, sector 0. El sector carga `BOOT` mediante el KERNAL y salta a `$1C01`. El runtime inicializa las pilas y la BSS sin pasar por BASIC 7.0.
- La MMU deja visible la RAM en `$4000–$BFFF` y conserva el KERNAL, el editor de pantalla y la E/S. CMD-OS selecciona el VDC de 80 columnas, desactiva la salida de 40 columnas y activa el modo de 2 MHz.
- `run-vice.ps1` monta la imagen con `x128`. El parámetro `-ReuSize` selecciona la capacidad de la REU virtual en KiB. El valor predeterminado es 16384. Los scripts apuntan a rutas de CC65 y VICE en `D:`. `boot.cmd` es otro lanzador de VICE con rutas absolutas y un disco adicional en el dispositivo 9.

## Consola y discos

| Entrada | Contrato actual |
| --- | --- |
| `A:`, `B:`, `C:`, `D:` | Seleccionan los dispositivos IEC 8, 9, 10 y 11. La consola inicia en `CMD-OS A:>`. |
| `DIR` | Muestra cabecera, archivos, tipos y tamaños. Cada archivo, el total utilizado y el espacio libre se expresan en bloques y KiB, a razón de 254 bytes útiles por bloque. |
| `RUN nombre[.cmd]` | Abre el archivo en la unidad activa y agrega `.cmd` si el nombre carece de punto. El argumento conserva sus caracteres originales. |
| `nombre[.cmd]` | Si no es un comando interno, busca una coincidencia exacta con el nombre de un archivo `.cmd` en la unidad activa y utiliza el mismo cargador que `RUN`. |
| `HELP` o tecla HELP | Carga `help.cmd` desde la unidad 8. La aplicación Bank 0 explica los comandos de la consola. |
| `VER` | Muestra la versión. |
| `EXIT` | Enumera las sesiones suspendidas y pide confirmación si las hay. Después indica que se puede apagar. |
| `M:` | Selecciona el volumen REU montado. |
| `FORMAT M: [/F]` | Inicializa el volumen tras pedir confirmación. `/F` permite reformatear un volumen montado y borrar sus archivos y sesiones suspendidas. |
| `COPY origen destino`, `DEL nombre` | Copia con unidades explícitas y borra en la unidad activa. |
| `STATUS` | Muestra el último código de salida CMDP. |

La comparación de comandos ignora diferencias entre las letras ASCII minúsculas y mayúsculas. Mantener el flujo de vuelta al prompt tras cada operación, incluidos los errores de disco o formato. Conservar el tratamiento PETSCII al imprimir nombres del disco y `\r\n` en las salidas de la consola.

## Arquitectura de memoria y servicios del kernel

El diseño acordado mantiene el **kernel de CMD-OS y el intérprete residentes**. El kernel tiene su base en Bank 0 y una puerta de entrada en RAM común visible desde ambos bancos. El intérprete usa los mismos servicios públicos que las aplicaciones y queda suspendido mientras se ejecuta un programa. Al finalizar, retoma la ejecución en memoria, sin recargarse desde disco. Esto es especialmente importante para encadenar comandos externos en archivos por lotes.

Solo se ejecuta **una aplicación a la vez**, sin multitarea ni aplicaciones residentes. Una aplicación pequeña se enlaza para la ventana temporal de Bank 0, junto al intérprete. Una aplicación grande se prepara para Bank 1 y puede usar la ventana temporal de Bank 0 como memoria auxiliar para textos, gráficos, música u otros datos. Los comandos por lotes y las aplicaciones interactivas pueden pertenecer a cualquiera de las dos clases. El banco depende del mapa para el que se enlazó el binario y de sus requisitos de memoria.

La ABI 1 ofrece servicios de consola, teclado y disco a los CMDP de ambos bancos. Durante el desarrollo se conserva una sola interfaz y se recompilan juntos el kernel y las aplicaciones cuando cambia el contrato. La compatibilidad con compilaciones anteriores se mantiene solo cuando se acuerda expresamente. Los archivos `.swp` usan el mismo identificador de ABI que los CMDP.

La ventana temporal de Bank 0 se presta por completo al programa activo. Puede contener una aplicación pequeña o los recursos de una grande. Al regresar al intérprete se invalida toda esa asignación y la siguiente ejecución puede reutilizarla. Para acceder a recursos desde Bank 1, se propone usar identificadores y copias por bloques a través del kernel. Un puntero C corriente expresa una dirección del mapa activo. El servicio que cruza bancos necesita conocer también el banco del búfer.

El prototipo integra el intérprete, el cargador y `src/kernel/cmdosk-dispatch.c` en el programa de arranque. La primera API de Bank 1 ya ofrece pantalla, teclado y archivos secuenciales. El préstamo de recursos de Bank 0 a aplicaciones de Bank 1 sigue pendiente.

Los módulos `cmdosk-*.c` en `src/kernel/` separan el despacho de la ABI,
la interfaz de texto, el selector de archivos, el acceso a disco, la copia,
los menús y la gestión de sesiones. Con `KRNL_TUI_MENU`, la aplicación entrega
los grupos y las opciones al kernel y recibe la opción elegida o cero si
el operador cancela. El kernel actualiza las filas afectadas cuando
cambia el resaltado. `EDIT` define sus opciones y ejecuta la acción recibida.

### Bank 1 y la API TUI

| Rango | Uso |
| --- | --- |
| `$0000–$0FFF` | RAM común temporal durante carga y ejecución. El puente empieza en `$0E00` y el buzón en `$0F70`. CMD-OS conserva y restaura la zona de sprites que usa el puente. |
| `$1000–$33FF` | Hasta 9 KiB para código, datos y BSS del CMDP Bank 1. |
| `$3400–$37FF` | Pila de C del CMDP Bank 1. |
| `$3800–$BFFF` | Búfer de texto de `EDIT`, con 34 KiB para el documento. |

Los códigos de operación se agrupan por servicio. La interfaz de texto ocupa `$20–$26`, teclado y sesiones `$30–$31`, disco `$40–$45` y `$47`, REU `$50` y argumentos `$60`. Los resultados propios de COPY ocupan `$30–$37` y `$3F`. Los estados DOS de las unidades IEC conservan su numeración.

La ABI 1 usa un buzón común con operaciones y un búfer de 128 bytes. El puente preserva la página cero de CC65 y cambia al mapa de Bank 0 antes de ejecutar un servicio. Los servicios usan la pila de C del intérprete. Al volver, el puente restaura el contexto del CMDP y su mapa. La puerta ofrece limpiar, pintar un tramo de texto, posicionar el cursor, leer una tecla, dibujar un diálogo y abrir, transferir y cerrar archivos de la unidad indicada en cada apertura. Las IRQ se habilitan dentro del servicio de Bank 0.

La paleta estándar vive en `src/kernel/cmdosk-abi.h`. Usa el cian sobre negro de BASIC 7.0 en 80 columnas y reserva el amarillo para los cuadros de diálogo y menús. El servicio TUI dibuja los bordes con caracteres de línea simple. `run edit` abre el editor VDC. F7 despliega los menús Archivo y Edición, que se recorren con las flechas y se activan con Enter. Esc o F7 cierra el menú. F1 abre y F3 guarda. `EDIT nombre` y `RUN edit nombre` abren el SEQ indicado en la unidad activa. Si no existe, el editor inicia un documento vacío y propone ese nombre al guardar. Un nombre inválido o un error de apertura se informa en la barra de estado. **Guardar como** se elige desde Archivo y carece de atajo. Para salir, elegir **Archivo → Salir**. F2 y F4 llevan al comienzo y al final. La barra de estado muestra `Archivo:` seguido del nombre actual.

### API de disco multicanal

Hay dos canales simultáneos compartidos entre IEC y `M:`. Cada canal conserva su unidad, modo y posición. `EDIT` usa el buzón de la ABI y `COPY` usa las mismas rutinas residentes para abrir el origen y el destino a la vez.

| Operación | Campos de entrada | Resultado |
| --- | --- | --- |
| `KRNL_DISK_READ_OPEN`, `KRNL_DISK_WRITE_OPEN` | Unidad en `y` (0 para `M:`, 8 a 11 para IEC), tipo PRG, SEQ o USR en `style` y nombre terminado en NUL en `data`. | `x` contiene el identificador del canal, 1 o 2. Si falla, `x` vale 0 y `result` indica el error. |
| `KRNL_DISK_READ` | Identificador en `x` y cantidad solicitada en `count`, hasta 128 bytes. | Los bytes se devuelven en `data`. `result` indica cuántos se leyeron, 0 al terminar o `KRNL_DISK_ERROR` si falla. |
| `KRNL_DISK_WRITE` | Identificador en `x`, cantidad en `count` y bytes en `data`. | `result` vale 0 si se escribieron todos los bytes. |
| `KRNL_DISK_CLOSE` | Identificador en `x`. Usar `count = 0` para completar la escritura o un valor distinto de cero para abortarla. | Libera el canal. En `M:`, una escritura abortada queda sin publicar. En IEC, el cierre puede dejar un archivo parcial y el programa que abrió el destino se ocupa de limpiarlo. |

El gestor bloquea el intercambio de sesiones mientras haya un canal de la API abierto o un canal de disco IEC registrado por el KERNAL.

### Regla general de los diálogos de archivo

Los nombres de archivo distinguen mayúsculas de minúsculas. CMD-OS usa minúsculas en los nombres que genera y conserva las mayúsculas que escribe el operador. Cada nombre admite hasta 16 caracteres PETSCII y solo puede contener letras de A a Z, dígitos, guiones y guiones bajos. Un único punto opcional separa el nombre de la extensión, con texto a ambos lados. Los espacios y las comillas se rechazan. En `RUN` y al ejecutar un CMDP por su nombre, el primer espacio separa el nombre del programa de sus argumentos. Si se omite la extensión, se agrega `.cmd` en minúsculas.

Hasta nuevo aviso, en todo CMD-OS, Enter con el nombre vacío abre el selector de la unidad activa, filtrado al tipo que corresponda a la operación. Las flechas arriba y abajo recorren la lista y Enter elige el archivo. Esc cancela desde el campo de nombre, el selector o un diálogo de confirmación. Un nombre ya escrito o sugerido se acepta con Enter. El editor utiliza el servicio común `KRNL_TUI_PICK_FILE` con el filtro SEQ. Al elegir un archivo distinto del actual como destino de guardado, el editor pide confirmar su reemplazo.

El editor lee archivos SEQ de la unidad activa, acepta finales CR, LF o CRLF y guarda en PETSCII con final CR. El límite de 34 KiB corresponde al documento una vez normalizados los saltos de línea. El diálogo de guardado propone el nombre actual. Al recibir un nombre por parámetro o en los diálogos de apertura y guardado, `EDIT` agrega `.txt` si el nombre carece de extensión. El nombre completo debe caber en 16 caracteres. Las extensiones explícitas y los nombres elegidos en el selector se conservan. Para reemplazarlo, el editor escribe y cierra `cmdos-edit.tmp`, conserva el original como `cmdos-edit.bak` durante el cambio de nombre y limpia el respaldo después de confirmar la operación. La unidad debe tener espacio para el temporal. El editor redibuja solo los caracteres que cambiaron y, en las ediciones que conservan el ajuste de línea, evita recorrer todo el documento visible.

## Aplicaciones externas

### Filosofía de uso de la memoria

`src/cmdos.c` conserva el estado del intérprete, incluida la unidad activa, mientras `RUN` presta a una aplicación una zona temporal de RAM. El cargador lee primero la cabecera, comprueba los requisitos y copia el ejecutable directamente a su dirección de enlace. Cierra los canales antes de llamar a la entrada del programa. La llamada es síncrona y el intérprete sigue desde el mismo punto cuando la aplicación retorna. Cada binario se enlaza para su ubicación y el formato actual carece de un mecanismo de reubicación. La bandera declara compatibilidad y no modifica las direcciones del binario.

Para los comandos externos por lotes, el intérprete deberá pasar argumentos y recibir un código de salida al recuperar el control. Así podrá seguir con la próxima línea y, más adelante, tomar decisiones según el resultado. La ABI 1 ya pasa argumentos y devuelve un código de salida que se puede consultar con `STATUS`. Quedan pendientes la sintaxis y el uso de este código en los lotes. También queda como ampliación la posibilidad de elegir un intérprete al arrancar mediante un archivo de configuración.

`src/apps/hello.c` valida el flujo mínimo. Muestra texto con `cputs`, obtiene los argumentos con `KRNL_ARGS`, espera una tecla con `KRNL_KEY_READ` y sale de `main`. La salida de `HELLO` permanece visible al volver al prompt.

El runtime de la aplicación (`src/runtime/cmdos-app-crt0.s`) selecciona el mapa CC65 de Bank 0, guarda la página cero usada por CC65 e inicializa la BSS y su propia pila de C antes de llamar a `main`. Al salir, restaura esa porción de página cero (incluido el puntero a la pila de C del intérprete), el puntero de la pila del procesador, `INIT_STATUS` y el mapa MMU. Escribe el valor de salida en `STATUS` y vuelve con `RTS`. El intérprete recoge el resultado devuelto por la aplicación y lo muestra con `STATUS`. Es un contrato de ejecución cooperativa de código nativo. La página cero se reutiliza por turnos, la pila de C de la aplicación tiene su propio espacio y la pila del procesador conserva la cadena de llamadas.

### Mapa efectivo y límites de Bank 0

| Rango | Uso o reserva actual |
| --- | --- |
| `$0002–$001B` | Reserva del enlazador para la página cero de CC65. El runtime guarda y restaura los bytes de trabajo de CC65. |
| Desde `$1C01` hasta el fin de la BSS | Área residente de CMD-OS. El final se calcula en cada compilación. |
| Fin de CMD-OS hasta `$73FF` | Margen para el crecimiento del kernel y del intérprete. |
| `$7400–$B3FF` | Área de enlace de aplicaciones de 16 KiB para código, datos, INIT y BSS. |
| `$B400–$B7FF` | Reserva para la pila de C de la aplicación, que crece hacia abajo desde `$B800`. |
| `$B800–$BFFF` | Reserva de la pila de C del intérprete con la configuración estándar instalada de CC65. |

El enlazador, el empaquetador y el cargador aplican `$B400` como límite exclusivo del área CMDP de Bank 0. La cabecera declara los bytes en disco. El enlazador comprueba también `INIT`, `BSS` y el límite inferior de la pila de C de la aplicación. El runtime inicia esa pila en `$B800`, en RAM visible con el mapa CC65. El build verifica que los segmentos residentes terminen antes de `$7400`. `run mem` muestra los segmentos de `boot.map`, las reservas y el margen para ampliar CMD-OS. La imagen incorpora `hello.cmd` y `mem.cmd` recompilados para la nueva dirección de carga.

En Bank 1 se mantiene la misma idea de área temporal. Lo que cambia es el banco en que viven el código y los datos de la aplicación. El intercambio con el kernel debe especificar a qué banco pertenece cada búfer, ya que una misma dirección puede referirse a RAM distinta al cambiar el mapa. La reserva para `M:` deberá formar parte del mismo presupuesto de memoria para preservar su contenido entre ejecuciones.

### Estado confirmado de M: y memoria

El arranque informa la VRAM del VDC y la capacidad de la REU. `M:` usa el formato v2. El banco 0 de la REU se reserva para los metadatos y cada banco siguiente aporta una ranura de 64 KiB, hasta un máximo de 16. Así, 128, 256 y 512 KiB ofrecen 1, 3 y 7 ranuras, respectivamente, y 16 MiB ofrece 16. Cada archivo admite
hasta 65.535 bytes. `DIR` muestra el tamaño real, el tipo IEC y los totales
asignado y libre por ranuras. Cada fila de `M:` muestra el tamaño en KiB,
el nombre entre comillas y el tipo IEC. Una línea en blanco separa la
cabecera de las entradas y otra separa las entradas de los totales. `COPY` admite PRG, SEQ y USR entre IEC y
`M:`, con unidades explícitas y sin sobreescribir el destino. El intérprete
carga `copy.cmd` desde la unidad 8. `EDIT` abre y guarda documentos SEQ
en la unidad activa y usa un temporal y un respaldo al reemplazar.

El kernel puede crecer hasta `$73FF`. Los CMDP Bank 0 enlazados para
una dirección anterior deben recompilarse para `$7400`. CMD-OS usa `$0400–$05F6`
del VIC-II para respaldar el área común y 36 celdas de 4 bits en
`$D800–$D823` para respaldar 18 bytes de las teclas de función. `MEM` calcula el margen para ampliar CMD-OS a partir del mapa de la compilación actual. La RAM de color se informa por separado.

### Cabecera CMDP v1

La cabecera ocupa 16 bytes, seguida por el ejecutable crudo sin dirección PRG ni stub BASIC. Los campos de dos bytes se guardan en orden little-endian.

Comparar la firma como bytes fijos. Los literales de caracteres de CC65 para C128 pueden tener valores PETSCII distintos de los bytes binarios de la cabecera. Mantener en minúsculas los nombres grabados en la imagen y conservar el nombre recibido por `RUN`.

| Offset hexadecimal | Campo | Comportamiento actual |
| --- | --- | --- |
| `00–03` | Firma | Bytes `43 4D 44 50` (`CMDP`). |
| `04` | Versión del formato | `01`. |
| `05` | Bancos | `01` para Bank 0, `02` para Bank 1 y `03` para ambos. `03` se ejecuta en Bank 0. |
| `06–07` | Dirección de carga | `$7400` para Bank 0 y `$1000` para Bank 1. |
| `08–09` | Desplazamiento de entrada | Relativo a la dirección de carga. El empaquetador escribe cero. El cargador exige que esté dentro del contenido. |
| `0A–0B` | Tamaño en bytes | Se exige una lectura completa y un máximo de `$4000` bytes para Bank 0 o `$2400` para Bank 1. |
| `0C` | Versión de API solicitada | El cargador exige `1` en ambos bancos. Recompilar los CMDP al cambiar la interfaz durante el desarrollo. |
| `0D–0F` | Reservados | El empaquetador escribe ceros. |

El enlazador reserva por defecto `$0400` bytes para la pila de C bajo `$B800`. Los segmentos de la aplicación, incluidas BSS e INIT, deben caber debajo de `$B400`.

## Material de referencia del repositorio

- `examples/` contiene demostraciones independientes en C y sus `.prg` de VDC. `lorem128` y `vdc_real_40_columns` ensayan 40 columnas con el VDC. `spotlight128` desplaza una vista de 40 columnas. `vdc_blink`, `vdc_blink2` y `vdc_random_art` trabajan con atributos y VRAM. `mandala128` genera patrones de texto. Los `.bat` disponibles compilan ejemplos individuales.
- [`examples/example.c`](../examples/example.c) es una demostración en C de la consola VDC de 80 columnas. Se puede compilar por separado con `cl65 -t c128 -O -o example-vdc.prg examples/example.c`.
- `examples/` conserva material de referencia en programas `.prg`, imágenes de disco y archivos comprimidos. Entre los binarios está [`examples/KeystoneKapers.prg`](../examples/KeystoneKapers.prg).
- `examples/crazy-lander/`, `examples/lander-c128/` y `examples/quad/` se compilan por separado. `examples/common/` contiene rutinas de texto VDC para los ejemplos. El build de CMD-OS toma sus fuentes de `src/`.
- `examples/c128 bootsect.7z` contiene `bootsect.128`, de 256 bytes. El proceso de compilación genera su propio sector.
- `LICENSE` contiene la licencia MIT del repositorio. `.gitignore` excluye `build/` y los `.o`. Los scripts de Git trabajan sobre `main`. `bajar-cambios.ps1` usa `pull --rebase`; `subir-cambios.ps1` agrega todo el árbol antes de crear un commit y hacer push.

## Historial de trabajo

1. Los commits `3c6e15a` y `aa08ad1` establecieron el arranque desde D64, la consola VDC de 80 columnas y la ejecución de aplicaciones externas CMDP. `92c5b9b` consolidó los fuentes del repositorio.
2. Se confirmó la arquitectura del proyecto. El kernel y el intérprete quedan residentes en Bank 0. Las aplicaciones pequeñas usan una reserva temporal en Bank 0 y las grandes usarán Bank 1. Ambos bancos deben poder llamar a la misma API del kernel. `RUN` ejecuta una aplicación por vez y retoma el intérprete al terminar.
3. Se detectó un solapamiento entre la ventana CMDP inicial y la pila de C del intérprete. Al probar `mem.cmd` en VICE también se encontró que la primera reserva de pila de la aplicación quedaba bajo ROM con el mapa activo de CC65. El mapa actual coloca las dos pilas en RAM visible, en reservas distintas.
4. Se incorporó `mem.cmd`, generado a partir del mapa de enlace de `boot.prg`. Muestra los rangos inclusivos, el tamaño en KiB de cada segmento, el total asignado y la RAM libre sin asignar dentro del mapa de CMD-OS.
5. Se corrigió la cabecera de `DIR` para mostrar el nombre, el ID y el tipo DOS. En VICE se verificó `"cmd-os" co 2a`, junto con el listado de archivos.
6. Se amplió el área CMDP a 24 KiB (`$5000–$AFFF`). Se recompilaron `hello.cmd` y `mem.cmd` para la nueva dirección. La pila CMDP quedó en `$B000–$B7FF` y la del intérprete en `$B800–$BFFF`. En VICE ambas aplicaciones se ejecutaron y volvieron al prompt. El último listado revisado mostró 36,32 KiB asignados y 4,70 KiB libres para ampliar CMD-OS (`$3D32–$4FFF`). Estas cifras se recalculan en cada compilación.
7. Se añadió `edit.cmd` como primer programa de Bank 1, junto con el puente MMU y los servicios TUI residentes. En VICE se abrió un SEQ de 33.792 bytes, se añadió un carácter al final y se verificaron los 33.793 bytes de la copia guardada. Al salir, el intérprete retomó el control y `dir` mostró ambos archivos.
8. Se adoptó el color de texto del arranque de BASIC 7.0 en el VDC, se agregaron bordes simples y menús desplegables, y se movió el nombre del documento a la barra de estado. El guardado propone el nombre actual y usa un temporal y un respaldo al reemplazar un archivo. Se redujo el redibujado al editar.
9. Se amplió el área de edición a las 80 columnas del VDC y a 21 filas visibles, desde la primera fila debajo del menú. Las líneas largas se ajustan visualmente a la siguiente fila sin agregar un salto al archivo.
10. F7 abre el menú. **Archivo → Nuevo** vuelve a un documento en blanco y sin nombre. **Archivo → Salir** vuelve al intérprete. Ambas opciones piden confirmación si hay cambios sin guardar.
11. El intérprete conserva `RUN` y permite iniciar un CMDP escribiendo solo su nombre, con o sin `.cmd`. Los comandos internos tienen prioridad.
12. Se incorporó un selector de archivos reutilizable en el kernel. El editor lista los SEQ de la unidad activa cuando se presiona Enter con el nombre vacío. Se adoptó esta regla para los futuros diálogos de archivo de CMD-OS y se actualizó el resumen ejecutivo del README.

13. Se verificaron en VICE el autoboot D71 con una 1571, el prompt en VDC a 2 MHz, la carga de HELLO, MEM y EDIT y el aviso de apagado de EXIT. MEM informa ambos bancos con los dos totales en una fila por banco. DIR muestra los KiB por archivo y los totales utilizado y libre. El intérprete omite los avisos de ejecución y finalización al lanzar un CMDP.

## Retomar el proyecto en otra máquina

Actualizar este documento y `README.md` únicamente cuando el usuario solicite explícitamente la actualización del archivo correspondiente. Las novedades de varias sesiones se pueden documentar juntas. Cuando se solicite actualizar los lineamientos, registrar las decisiones, el estado comprobado y el siguiente paso.

Usar este documento versionado para trasladar el contexto entre máquinas. En la otra máquina, clonar el repositorio o actualizar la copia existente con `git pull --ff-only origin main` desde `main`. Abrir esa carpeta en Codex y empezar con este mensaje:

> Lee `doc/lineamientos-cmd-os.md` y usa su historial como contexto. Revisa el estado del repositorio y retomemos CMD-OS desde allí.

La imagen D71 y los demás resultados compilados se regeneran con `build.ps1`, porque `build/` está excluido de Git. Con las rutas predeterminadas de la máquina actual se usa:

```powershell
.\build.ps1
.\run-vice.ps1
```

Adaptar las rutas a la instalación de cada equipo. La comprobación básica usa `dir`, `run hello`, `run mem` y `run edit`. Escribir en minúsculas los nombres de los archivos tal como figuran en el disco. Quedan pendientes el préstamo de recursos entre bancos, los lotes y las pruebas de recuperación de sesiones en hardware real.

## Orden de evolución propuesto

1. Completar la validación del arranque autónomo desde D71 en hardware real y verificar la apertura y el guardado de archivos SEQ con `EDIT` en la imagen nueva.
2. Mantener las reservas del intérprete y de las aplicaciones separadas. Probar el puente y el editor también en hardware real.
3. Adaptar las aplicaciones de Bank 0 a los servicios comunes y acordar una ABI para compartir recursos entre bancos.
4. Probar el uso de la ventana temporal de Bank 0 para recursos de una aplicación Bank 1 y la reutilización de esa misma ventana por una aplicación Bank 0 después del retorno.
5. Implementar los lotes. Validar una secuencia de comandos externos que conserve el estado del intérprete entre ejecuciones.
6. Verificar en hardware real la suspensión y reanudación alternada de dos sesiones de `EDIT` con documentos sin guardar. Verificar la recuperación tras reiniciar con la REU conservada y agregar el controlador opcional para NeoRAM Remix.
