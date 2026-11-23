# Lander Oscar para Commodore 128

Port nativo del juego original `doc/Lander Oscar DOS/GRAFICO.CPP`.

Usa `c128-vdc.tgi` de CC65, incluido dentro de la imagen D64 y cargado dinámicamente al arrancar: 640x200 píxeles, dos colores y compatible con el VDC básico de 16 KB. No usa `c128-vdc2.tgi`, que requiere 64 KB de VRAM.

El programa muestra el progreso de carga del TGI antes de entrar al modo gráfico. La escena conserva el fondo estrellado, suelo lunar, plataforma, Lander, meteorito, colisiones, gravedad, explosión y aterrizaje. Los objetos son sprites software y la escena se redibuja en cada cuadro para eliminar rastros. La pantalla DOS de 640x480 se proyecta verticalmente a 640x200, que es la resolución gráfica disponible sin expansión. Al terminar, descarga el TGI y restaura explícitamente el modo texto VDC.

Controles: flechas o `W/A/S/D` para mover el Lander; `ESC` para abandonar. El lanzador de VICE usa `-autostart` para cargar el programa automáticamente.

## Compilar y ejecutar

```powershell
.\build.ps1
.\run-vice.ps1
```

Las rutas por defecto son `D:\Program Files (x86)\cc65` y `D:\Program Files (x86)\GTK3VICE-3.9-win64`; ambas pueden cambiarse con `-Cc65Home` y `-ViceHome`. El resultado queda en `build/lander-c128.prg` y `build/lander-c128.d64`.
