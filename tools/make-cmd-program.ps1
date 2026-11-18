param(
    [Parameter(Mandatory = $true)]
    [string] $InputFile,
    [Parameter(Mandatory = $true)]
    [string] $OutputFile,
    [Parameter(Mandatory = $true)]
    [ValidateSet('BANK0', 'BANK1', 'BOTH')]
    [string] $Bank,
    [Parameter(Mandatory = $false)]
    [int] $LoadAddress = 0x5000,
    [Parameter(Mandatory = $false)]
    [int] $ApiVersion = 1
)

$ErrorActionPreference = 'Stop'

$payload = [System.IO.File]::ReadAllBytes($InputFile)
if ($payload.Length -eq 0) {
    throw "El programa $InputFile esta vacio."
}
if ($LoadAddress -lt 0 -or $LoadAddress -gt 0xFFFF) {
    throw "La direccion de carga no es valida: $LoadAddress."
}
if ($Bank -ne 'BANK1' -and ($LoadAddress -ne 0x5000 -or $payload.Length -gt (0xB000 - 0x5000))) {
    throw 'El programa compatible con BANK0 debe cargar en $5000 y terminar antes de $B000.'
}
if ($Bank -eq 'BANK1' -and ($LoadAddress -ne 0x1000 -or $payload.Length -gt 0x2400 -or $ApiVersion -ne 2)) {
    throw 'BANK1 requiere la API 2 y un programa de hasta $2400 bytes enlazado en $1000.'
}
if ($ApiVersion -lt 0 -or $ApiVersion -gt 255) {
    throw "La version de API no es valida: $ApiVersion."
}

$flags = switch ($Bank) {
    'BANK0' { 0x01 }
    'BANK1' { 0x02 }
    'BOTH'  { 0x03 }
}

# CMDP v1:
# 00-03 magic, 04 version, 05 flags, 06-07 load address,
# 08-09 entry offset, 0A-0B payload size, 0C API version,
# 0D-0F reserved.
$header = New-Object byte[] 16
$header[0] = 0x43 # C
$header[1] = 0x4D # M
$header[2] = 0x44 # D
$header[3] = 0x50 # P
$header[4] = 1
$header[5] = $flags
$header[6] = [byte]($LoadAddress -band 0xFF)
$header[7] = [byte](($LoadAddress -shr 8) -band 0xFF)
$header[8] = 0
$header[9] = 0
$header[10] = [byte]($payload.Length -band 0xFF)
$header[11] = [byte](($payload.Length -shr 8) -band 0xFF)
$header[12] = [byte]$ApiVersion

$output = New-Object byte[] ($header.Length + $payload.Length)
[Array]::Copy($header, 0, $output, 0, $header.Length)
[Array]::Copy($payload, 0, $output, $header.Length, $payload.Length)
[System.IO.File]::WriteAllBytes($OutputFile, $output)
Write-Host "Programa CMDP creado: $OutputFile"
