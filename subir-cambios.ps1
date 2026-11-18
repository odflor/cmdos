param(
    [Parameter(Mandatory = $false)]
    [string] $Mensaje = ''
)

$ErrorActionPreference = 'Stop'

Set-Location (Split-Path -Parent $MyInvocation.MyCommand.Path)

$branch = git branch --show-current
if ($LASTEXITCODE -ne 0) {
    throw 'No se pudo consultar Git. Verifica que Git esté instalado.'
}
if ($branch -ne 'main') {
    throw "La rama actual es '$branch'. Este script solo trabaja con la rama main."
}

$status = git status --porcelain
if ($status -match '^(UU|AA|DD|AU|UA|DU|UD) ') {
    Write-Host 'Hay conflictos sin resolver. No se hará ningún commit ni push.' -ForegroundColor Red
    git status
    exit 1
}
if ([string]::IsNullOrWhiteSpace($status)) {
    Write-Host 'No hay cambios locales para subir.' -ForegroundColor Yellow
    exit 0
}

if ([string]::IsNullOrWhiteSpace($Mensaje)) {
    $Mensaje = Read-Host 'Escribe el mensaje del commit'
}
if ([string]::IsNullOrWhiteSpace($Mensaje)) {
    throw 'El mensaje del commit no puede estar vacío.'
}

Write-Host 'Preparando cambios...' -ForegroundColor Cyan
git add -A
git commit -m $Mensaje
if ($LASTEXITCODE -ne 0) {
    throw 'No se pudo crear el commit.'
}

Write-Host 'Actualizando referencias remotas...' -ForegroundColor Cyan
git fetch origin
if ($LASTEXITCODE -ne 0) {
    throw 'No se pudieron descargar las referencias remotas.'
}

Write-Host 'Subiendo cambios a origin/main...' -ForegroundColor Cyan
git push origin main
if ($LASTEXITCODE -ne 0) {
    Write-Host 'El push falló. No se usó force; revisa si hay cambios remotos pendientes.' -ForegroundColor Red
    exit 1
}

Write-Host 'Cambios subidos correctamente.' -ForegroundColor Green
