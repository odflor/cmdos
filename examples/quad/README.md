# quad

Prueba mínima del modo gráfico monocromático de 80 columnas del C128.

El programa avisa en modo texto mientras carga `c128-vdc.tgi` desde la propia imagen D64, confirma que fue cargado antes de entrar al modo gráfico, inicializa TGI y dibuja un cuadrado centrado en 640x200 con un patrón de rombos. Al pulsar una tecla muestra un mensaje de salida y restaura explícitamente el modo texto VDC. La imagen usa solamente nombres de archivo en minúsculas.

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
powershell -ExecutionPolicy Bypass -File .\run-vice.ps1
```
