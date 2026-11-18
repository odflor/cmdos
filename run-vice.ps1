$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$vice = 'D:\GTK3VICE-3.10-win64\bin\x128.exe'
$disk = Join-Path $projectRoot 'build\cmd-os.d64'

if (-not (Test-Path $disk)) {
    & (Join-Path $projectRoot 'build.ps1')
}

& $vice -8 $disk
