$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cc65Bin = 'D:\cc65\bin'
$viceBin = 'D:\GTK3VICE-3.10-win64\bin'
$buildDir = Join-Path $projectRoot 'build'
$source = Join-Path $projectRoot 'src\cmdos.c'
$program = Join-Path $buildDir 'boot.prg'
$appSource = Join-Path $projectRoot 'src\apps\hello.c'
$appCrt0 = Join-Path $projectRoot 'src\runtime\cmdos-app-crt0.s'
$appCfg = Join-Path $projectRoot 'cfg\cmdos-app-bank0.cfg'
$appRaw = Join-Path $buildDir 'hello.raw'
$appCmd = Join-Path $buildDir 'hello.cmd'
$disk = Join-Path $buildDir 'cmd-os.d64'

New-Item -ItemType Directory -Force $buildDir | Out-Null

& (Join-Path $cc65Bin 'cl65.exe') -t c128 -O -o $program $source
if ($LASTEXITCODE -ne 0) { throw "CC65 no pudo compilar CMD-OS." }

# Build the first external application without the BASIC startup header.
& (Join-Path $cc65Bin 'cl65.exe') -t c128 -O -C $appCfg -o $appRaw $appSource $appCrt0
if ($LASTEXITCODE -ne 0) { throw "CC65 no pudo compilar la aplicacion HELLO." }

& (Join-Path $projectRoot 'tools\make-cmd-program.ps1') `
    -InputFile $appRaw `
    -OutputFile $appCmd `
    -Bank BANK0
if ($LASTEXITCODE -ne 0) { throw "No se pudo crear el ejecutable CMDP de HELLO." }

if (Test-Path $disk) { Remove-Item -LiteralPath $disk -Force }
$c1541 = Join-Path $viceBin 'c1541.exe'
& $c1541 -format 'cmd-os,co' d64 $disk `
    -write $program boot `
    -write $appCmd hello.cmd
if ($LASTEXITCODE -ne 0) { throw "VICE c1541 no pudo crear la imagen D64." }

& (Join-Path $projectRoot 'tools\make-boot-sector.ps1') -DiskImage $disk -Program $program
if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar el sector de autoboot C128." }

Write-Host "Imagen creada: $disk"
Write-Host "Arranque de VICE: $viceBin\x128.exe -8 $disk"
