param(
    [Parameter(Mandatory = $true)]
    [string] $DiskImage,
    [Parameter(Mandatory = $true)]
    [string] $BootSector
)

$ErrorActionPreference = 'Stop'
$image = [IO.File]::ReadAllBytes($DiskImage)
$sector = [IO.File]::ReadAllBytes($BootSector)
if ($image.Length -ne 349696 -or $sector.Length -ne 256) {
    throw 'Se requiere una imagen D71 y un sector de autoboot de 256 bytes.'
}

# Track 1, sector 0 is the first sector in the image.
# Reserve it in the track 1 BAM entry at track 18, sector 0.
$trackOneBam = 17 * 21 * 256 + 4
if (($image[$trackOneBam + 1] -band 1) -eq 0) {
    throw 'La pista 1, sector 0 ya esta ocupada.'
}
$image[$trackOneBam]--
$image[$trackOneBam + 1] = $image[$trackOneBam + 1] -band 0xFE
[Array]::Copy($sector, $image, $sector.Length)
[IO.File]::WriteAllBytes($DiskImage, $image)
Write-Host "Sector de autoboot C128 instalado en $DiskImage"
