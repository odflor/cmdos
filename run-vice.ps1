param(
    [string] $ViceBin = 'D:\temp\GTK3VICE-3.9-win64\bin',
    [ValidateSet(128, 256, 512, 1024, 2048, 4096, 8192, 16384)]
    [int] $ReuSize = 16384
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$vice = Join-Path $ViceBin 'x128.exe'
$disk = Join-Path $projectRoot 'build\cmd-os.d71'

if (-not (Test-Path $disk)) {
    & (Join-Path $projectRoot 'build.ps1') -ViceBin $ViceBin
}

& $vice -drive8type 1571 -8 $disk -VDC16KB -80col -reu -reusize $ReuSize
