$ErrorActionPreference = 'Stop'

Set-Location (Split-Path -Parent $MyInvocation.MyCommand.Path)

Write-Host 'Comprobando estado del repositorio...' -ForegroundColor Cyan
$status = git status --porcelain --branch
if ($LASTEXITCODE -ne 0) {
    throw 'No se pudo consultar Git. Verifica que Git esté instalado.'
}

if ($status -match '^(UU|AA|DD|AU|UA|DU|UD) ') {
    Write-Host 'Hay conflictos sin resolver. Resuélvelos antes de bajar cambios.' -ForegroundColor Red
    git status
    exit 1
}

$branch = git branch --show-current
if ($branch -ne 'main') {
    throw "La rama actual es '$branch'. Este script solo trabaja con la rama main."
}

Write-Host 'Descargando cambios desde origin/main...' -ForegroundColor Cyan
git pull --rebase origin main
if ($LASTEXITCODE -ne 0) {
    Write-Host 'La actualización no terminó. Revisa los conflictos indicados por Git.' -ForegroundColor Red
    git status
    exit 1
}

Write-Host 'Cambios bajados correctamente.' -ForegroundColor Green
