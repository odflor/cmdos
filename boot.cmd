@echo off
setlocal

rem Montar CMD-OS en una 1571 como unidad 8.
set "vice=D:\temp\GTK3VICE-3.9-win64\bin\x128.exe"
set "disk=%~dp0build\cmd-os.d71"
set "tutorial=D:\Downloads\commodore_stuff\c128\c128_tutorial_disk.d64"
if exist "%tutorial%" (
    "%vice%" -drive8type 1571 -8 "%disk%" -9 "%tutorial%"
) else (
    "%vice%" -drive8type 1571 -8 "%disk%"
)

endlocal
