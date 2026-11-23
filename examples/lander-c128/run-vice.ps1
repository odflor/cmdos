param([string] $ViceHome = 'D:\Program Files (x86)\GTK3VICE-3.9-win64')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$disk = Join-Path $root 'build\lander-c128.d64'
$x128 = Join-Path $ViceHome 'bin\x128.exe'
if (-not (Test-Path $disk)) { & (Join-Path $root 'build.ps1') }
& $x128 -80col -autostartprgmode 1 -autostart $disk
