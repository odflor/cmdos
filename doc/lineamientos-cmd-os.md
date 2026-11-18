# Lineamientos de CMD-OS

Documento de continuidad del proyecto. Integra los lineamientos del repositorio con las decisiones de diseño de las conversaciones de trabajo. El intérprete se identifica como **CMD-OS 0.2.0**. Las secciones de arquitectura describen el diseño acordado y los contratos del prototipo se documentan con el código como referencia. El orden de evolución al final es una propuesta.

## Plataforma y arranque

- Desarrollar para Commodore 128 en modo nativo, con CC65 (`-t c128`), C para el intérprete y ca65 para el arranque de aplicaciones. La consola usa el VDC de 80 columnas y las rutinas de pantalla, teclado y disco de CC65/KERNAL.
- El artefacto principal es `build/cmd-os.d64`, montado en el dispositivo 8. `build.ps1` compila y empaqueta los programas y escribe `boot`, `hello.cmd` y `mem.cmd` con `c1541`. `tools/make-boot-sector.ps1` instala el sector de autoboot en pista 1, sector 0. El sector lleva la firma `CBM` y transfiere el control a BASIC con `RUN"BOOT"` mediante la entrada ROM `$AFA5`. Preservar este autoboot y el stub de BASIC de `boot.prg`.
- `run-vice.ps1` monta la imagen con `x128`. Los scripts apuntan a rutas de CC65 y VICE en `D:`. `boot.cmd` es otro lanzador de VICE con rutas absolutas y un disco adicional en el dispositivo 9.

## Consola y discos

| Entrada | Contrato actual |
| --- | --- |
| `A:`, `B:`, `C:`, `D:` | Seleccionan los dispositivos IEC 8, 9, 10 y 11. La consola inicia en `CMD-OS A:>`. |
| `DIR` | Lee la unidad activa y muestra cabecera, archivos, tipos, tamaños y bloques libres. |
| `RUN nombre[.cmd]` | Abre el archivo en la unidad activa y agrega `.cmd` si el nombre carece de punto. El argumento conserva sus caracteres originales. |
| `HELP`, `VER` | Muestran los comandos y la versión. |
| `EXIT` | Devuelve el control a BASIC 7.0. |
| `M:` | Informa que la unidad está reservada para un futuro disco virtual. |

La comparación de comandos ignora diferencias entre las letras ASCII minúsculas y mayúsculas. Mantener el flujo de vuelta al prompt tras cada operación, incluidos los errores de disco o formato. Conservar el tratamiento PETSCII al imprimir nombres del disco y `\r\n` en las salidas de la consola.

## Arquitectura de memoria y servicios del kernel

El diseño acordado mantiene el **kernel de CMD-OS y el intérprete residentes**. El kernel tiene su base en Bank 0 y una puerta de entrada en RAM común visible desde ambos bancos. El intérprete usa los mismos servicios públicos que las aplicaciones y queda suspendido mientras se ejecuta un programa. Al finalizar, retoma la ejecución en memoria, sin recargarse desde disco. Esto es especialmente importante para encadenar comandos externos en archivos por lotes.

Solo se ejecuta **una aplicación a la vez**, sin multitarea ni aplicaciones residentes. Una aplicación pequeña se enlaza para la ventana temporal de Bank 0, junto al intérprete. Una aplicación grande se prepara para Bank 1 y puede usar la ventana temporal de Bank 0 como memoria auxiliar para textos, gráficos, música u otros datos. Los comandos por lotes y las aplicaciones interactivas pueden pertenecer a cualquiera de las dos clases. El banco depende del mapa para el que se enlazó el binario y de sus requisitos de memoria.

La API común debe empezar con servicios de consola, teclado y disco. La puerta de entrada debe poder seleccionar el mapa apropiado para el servicio y restaurar el de la aplicación antes de devolverle el control. Queda por fijar qué parte del kernel cabrá en RAM común, la ubicación y versión de la ABI, las convenciones de llamada y la preservación de página cero, pilas, MMU e interrupciones.

La ventana temporal de Bank 0 se presta por completo al programa activo. Puede contener una aplicación pequeña o los recursos de una grande. Al regresar al intérprete se invalida toda esa asignación y la siguiente ejecución puede reutilizarla. Para acceder a recursos desde Bank 1, se propone usar identificadores y copias por bloques a través del kernel. Un puntero C corriente expresa una dirección del mapa activo. El servicio que cruza bancos necesita conocer también el banco del búfer.

El prototipo actual integra el intérprete y el cargador en el programa de arranque. La API pública, la puerta de entrada común, la ejecución en Bank 1 y el préstamo de recursos siguen pendientes.

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
| `05` | Bancos | `01` para Bank 0, `02` para Bank 1 y `03` para ambos. Para ejecutar se exige el bit de Bank 0. `03` se ejecuta en Bank 0. |
| `06–07` | Dirección de carga | `$5000`, única dirección aceptada. |
| `08–09` | Desplazamiento de entrada | Relativo a la dirección de carga. El empaquetador escribe cero. El cargador exige que esté dentro del contenido. |
| `0A–0B` | Tamaño en bytes | Se exige una lectura completa y un máximo de `$6000` bytes, con `$B000` como límite exclusivo. |
| `0C` | Versión de API solicitada | El empaquetador escribe `1` por defecto y el cargador admite `0` y `1`. La API común sigue pendiente. |
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

## Retomar el proyecto en otra máquina

Usar este documento versionado para trasladar el contexto entre máquinas. Al cerrar una sesión, actualizar aquí las decisiones, el estado comprobado y el siguiente paso. Revisar los cambios, incluir el documento en un commit y publicar la rama en GitHub. En la otra máquina, clonar el repositorio o actualizar la copia existente con `git pull --ff-only origin main` desde `main`. Abrir esa carpeta en Codex y empezar con este mensaje:

> Lee `doc/lineamientos-cmd-os.md` y usa su historial como contexto. Revisa el estado del repositorio y retomemos CMD-OS desde allí.

La imagen D64 y los demás resultados compilados se regeneran con `build.ps1`, porque `build/` está excluido de Git. En la máquina donde se hicieron estas pruebas se usó:

```powershell
.\build.ps1 -Cc65Bin C:\cc65\bin -ViceBin C:\GTK3VICE-3.7-win32\bin
& 'C:\GTK3VICE-3.7-win32\bin\x128.exe' -80col -8 .\build\cmd-os.d64
```

Adaptar las rutas a la instalación de cada equipo. La comprobación básica usa `dir`, `run hello` y `run mem`. Escribir en minúsculas los nombres de los archivos tal como figuran en el disco. En una prueba, `RUN HELLO` falló al abrir el CMDP con el nombre en mayúsculas. Para continuar con el diseño quedan pendientes la API pública del kernel, las llamadas entre bancos, la carga de aplicaciones en Bank 1 y el disco virtual `M:`.

## Orden de evolución propuesto

1. Validar una imagen recién generada en `x128`. Comprobar el autoboot desde el dispositivo 8, `ver`, `dir`, `run hello`, `run mem`, el regreso al prompt, las unidades, el manejo de archivos inválidos y `exit` a BASIC.
2. Mantener el mapa de Bank 0 con las zonas del intérprete y de la aplicación separadas. Reservar el espacio de Bank 1 para las aplicaciones grandes y definir qué estado de pantalla y sistema debe preservar cada aplicación al retornar.
3. Definir la API común del kernel y su punto de entrada para ambos bancos. Validar primero una llamada desde el intérprete y desde `hello.cmd` en Bank 0. Extender después el cargador y el runtime a Bank 1 y comprobar que una aplicación pueda llamar al mismo servicio, continuar su ejecución y devolver el control al intérprete.
4. Probar el uso de la ventana temporal de Bank 0 para recursos de una aplicación Bank 1 y la reutilización de esa misma ventana por una aplicación Bank 0 después del retorno.
5. Pasar argumentos y devolver códigos de salida. Validar una secuencia de comandos externos que conserve el estado del intérprete entre ejecuciones.
6. Definir la capacidad, el formato y el acceso del disco virtual `M:`. Incorporarlo a `DIR` y `RUN` cuando disponga de lectura de directorio y archivos.
