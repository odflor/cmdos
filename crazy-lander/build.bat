@echo off
setlocal

set "CC65_BIN=C:\cc65\bin"
set "CC65=%CC65_BIN%\cl65.exe"
set "OUTPUT=crazylander.prg"

if not exist "%CC65%" (
    echo ERROR: no se encontro CC65 en %CC65_BIN%
    exit /b 1
)

echo Compilando Crazy Lander para Commodore 128...
"%CC65%" -t c128 -O -o "%OUTPUT%" main.c

if errorlevel 1 (
    echo ERROR: la compilacion fallo.
    exit /b 1
)

echo Compilacion correcta: %OUTPUT%
endlocal
