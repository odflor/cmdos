param(
    [string] $Cc65Home = 'D:\Program Files (x86)\cc65',
    [string] $ViceHome = 'D:\Program Files (x86)\GTK3VICE-3.9-win64'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root 'build'
$source = Join-Path $root 'src\quad.c'
$program = Join-Path $build 'quad.prg'
$disk = Join-Path $build 'quad.d64'
$cl65 = Join-Path $Cc65Home 'bin\cl65.exe'
$c1541 = Join-Path $ViceHome 'bin\c1541.exe'
$driver = Join-Path $Cc65Home 'target\c128\drv\tgi\c128-vdc.tgi'

if (-not (Test-Path $cl65)) { throw "No se encontro CC65: $cl65" }
if (-not (Test-Path $c1541)) { throw "No se encontro VICE c1541: $c1541" }
if (-not (Test-Path $driver)) { throw "No se encontro el driver TGI: $driver" }

New-Item -ItemType Directory -Force $build | Out-Null
& $cl65 -t c128 -O -o $program $source
if ($LASTEXITCODE -ne 0) { throw 'CC65 no pudo compilar quad.' }

if (Test-Path $disk) { Remove-Item -LiteralPath $disk -Force }
& $c1541 -format 'quad,qd' d64 $disk -write $program quad
if ($LASTEXITCODE -ne 0) { throw 'VICE c1541 no pudo crear quad.d64.' }
& $c1541 $disk -write $driver 'c128-vdc.tgi'
if ($LASTEXITCODE -ne 0) { throw 'VICE c1541 no pudo incluir c128-vdc.tgi.' }

Write-Host "Programa creado: $program"
Write-Host "Imagen creada:   $disk"
