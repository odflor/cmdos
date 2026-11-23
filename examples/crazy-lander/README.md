# Crazy Lander

Prototipo de un clon de Lander/Jupiter Lander para Commodore 128 en modo C128
nativo, compilado con CC65.

## Primera etapa

- Ejecutable `.PRG` autocontenido.
- Pantalla de presentación, juego y score.
- Modo de vídeo de 40 columnas basado en el VIC-II.
- Base preparada para añadir tiles, sprites, sonido SID y la lógica de vuelo.

## Compilación

Desde esta carpeta:

```powershell
C:\cc65\bin\cl65.exe -t c128 -O -o CRAZYLANDER.PRG main.c
```

El archivo resultante puede cargarse directamente en VICE con `x128.exe`.

## Controles actuales del prototipo

- `ESPACIO`: avanzar desde la presentación o reiniciar desde score.
- `S`: simular fin de misión y mostrar score.
- `Q`: salir.

