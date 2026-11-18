param(
    [Parameter(Mandatory = $true)]
    [string] $DiskImage,
    [Parameter(Mandatory = $true)]
    [string] $Program
)

$ErrorActionPreference = 'Stop'

# El C128 inspecciona la pista 1, sector 0 al arrancar.
$image = [System.IO.File]::ReadAllBytes($DiskImage)
if ($image.Length -lt 256) {
    throw "La imagen D64 es demasiado pequena."
}

$bootSector = New-Object byte[] 256

# Cabecera oficial de autoboot C128: firma CBM y sin sectores adicionales.
$bootSector[0] = 0x43
$bootSector[1] = 0x42
$bootSector[2] = 0x4D
$bootSector[3] = 0x00
$bootSector[4] = 0x00
$bootSector[5] = 0x00
$bootSector[6] = 0x00

# Los campos del protocolo son PETSCII. El nombre almacenado en la imagen
# sigue siendo "boot"; aquí se usa la representación PETSCII esperada por ROM.
$title = [System.Text.Encoding]::ASCII.GetBytes('CMD-OS')
[Array]::Copy($title, 0, $bootSector, 7, $title.Length)
$titleTerminator = 7 + $title.Length
$bootSector[$titleTerminator] = 0x00

# No usamos el campo filename: el código del sector se ejecutará en el banco
# de autoboot y entregará el trabajo a BASIC mediante BOOT"BOOT". Esto es
# necesario porque el PRG de CC65 empieza con un stub BASIC y su entrada real
# no coincide con la dirección de carga $1C01.
$bootSector[$titleTerminator + 1] = 0x00
$code = $titleTerminator + 2
$command = 'RUN"BOOT"'
$commandBytes = [System.Text.Encoding]::ASCII.GetBytes($command)

# JSR $AFA5 (J_EXECUTE_A_LINE), con X/Y apuntando a la orden BASIC.
$bootSector[$code] = 0xA2
# J_EXECUTE_A_LINE recibe el byte inmediatamente anterior a la orden.
$bootSector[$code + 1] = $code + 6
$bootSector[$code + 2] = 0xA0
$bootSector[$code + 3] = 0x0B
$bootSector[$code + 4] = 0x4C
$bootSector[$code + 5] = 0xA5
$bootSector[$code + 6] = 0xAF
[Array]::Copy($commandBytes, 0, $bootSector, $code + 7, $commandBytes.Length)
$bootSector[$code + 7 + $commandBytes.Length] = 0x00

[Array]::Copy($bootSector, 0, $image, 0, 256)
[System.IO.File]::WriteAllBytes($DiskImage, $image)
Write-Host "Sector de autoboot C128 instalado en $DiskImage"
