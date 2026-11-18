@echo off
setlocal

rem Iniciar Commodore 128, activar la pantalla VDC de 80 columnas
rem y montar la imagen existente del CMD-OS como unidad 8.
"D:\GTK3VICE-3.10-win64\bin\x128.exe" -80col -8 "D:\c128fuentes\build\cmd-os.d64" -9 "d:\Downloads\commodore_stuff\c128\c128_tutorial_disk.d64"

endlocal
