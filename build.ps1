param(
    [string] $Cc65Bin = 'D:\temp\cc65\bin',
    [string] $ViceBin = 'D:\temp\GTK3VICE-3.9-win64\bin',
    [string] $ImageName = 'cmd-os.d71'
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $projectRoot 'build'
$source = Join-Path $projectRoot 'src\cmdos.c'
$program = Join-Path $buildDir 'boot.prg'
$map = Join-Path $buildDir 'boot.map'
$memoryInclude = Join-Path $buildDir 'cmdos-memory.inc'
$appCrt0 = Join-Path $projectRoot 'src\runtime\cmdos-app-crt0.s'
$appCfg = Join-Path $projectRoot 'cfg\cmdos-app-bank0.cfg'
$bank1Cfg = Join-Path $projectRoot 'cfg\cmdos-app-bank1.cfg'
$bank1Crt0 = Join-Path $projectRoot 'src\runtime\cmdos-bank1-crt0.s'
if ($ImageName -notmatch '^[\w-]+\.d71$') { throw 'Indica un nombre de imagen D71.' }
$disk = Join-Path $buildDir $ImageName
$bootSector = Join-Path $buildDir 'boot-sector.bin'

New-Item -ItemType Directory -Force $buildDir | Out-Null

$cl65 = Join-Path $Cc65Bin 'cl65.exe'
& $cl65 -t c128 -O -C (Join-Path $projectRoot 'cfg\cmdos.cfg') -m $map -o $program `
    (Join-Path $projectRoot 'src\runtime\cmdos-crt0.s') $source `
    (Join-Path $projectRoot 'src\kernel\cmdosk-dispatch.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-tui.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-picker.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-menu.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-disk.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-copy.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-swap.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-sessions.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-bank1.c') `
    (Join-Path $projectRoot 'src\kernel\cmdosk-keyboard.s') `
    (Join-Path $projectRoot 'src\kernel\bank1-gate.s') `
    (Join-Path $projectRoot 'src\kernel\reu.s') `
    (Join-Path $projectRoot 'src\kernel\vdc-probe.s')
if ($LASTEXITCODE -ne 0) { throw "CC65 no pudo compilar CMD-OS." }

# Extract the segment table; LOADADDR is the PRG file prefix, not a RAM segment.
$mapText = [IO.File]::ReadAllText($map)
$segmentTable = [regex]::Match($mapText, '(?ms)^Segment list:\s*\r?\n(.*?)^Exports list').Groups[1].Value
$segments = [regex]::Matches($segmentTable, '(?m)^([A-Z][A-Z0-9_]*)[ \t]+([0-9A-F]{6})[ \t]+([0-9A-F]{6})[ \t]+([0-9A-F]{6})[ \t]+[0-9A-F]{5}\s*$')
if ('STARTUP' -notin @($segments | ForEach-Object { $_.Groups[1].Value })) {
    throw 'No se pudo leer la tabla de segmentos de boot.map.'
}
$residentEnd = 0
$memoryRows = foreach ($segment in $segments) {
    $name, $start, $end, $size = $segment.Groups[1..4].Value
    if ($name -ne 'LOADADDR' -and $size -ne '000000') {
        $endAddress = [Convert]::ToInt32($end, 16)
        if ($endAddress -ge 0x7400) {
            throw "El segmento $name de CMD-OS invade la zona reservada para aplicaciones."
        }
        $residentEnd = [Math]::Max($residentEnd, $endAddress)
        '    {{ "{0}", 0x{1}, 0x{2}, 0 }},' -f $name, $start, $end
    }
}
if ($residentEnd -lt 0x73FF) {
    $memoryRows += '    {{ "RAM libre para CMD-OS", 0x{0:X4}, 0x73FF, 1 }},' -f ($residentEnd + 1)
}
$memoryRows += '    { "VIC-II: respaldo area comun", 0x0400, 0x05F6, 0 },',
    '    { "Area CMDP (reserva)", 0x7400, 0xB3FF, 0 },',
    '    { "Pila C CMDP (reserva)", 0xB400, 0xB7FF, 0 },',
    '    { "Pila C CMD-OS (reserva)", 0xB800, 0xBFFF, 0 },'
Set-Content -LiteralPath $memoryInclude -Value $memoryRows -Encoding Ascii

foreach ($app in 'hello', 'mem', 'copy', 'help') {
    $appSource = Join-Path $projectRoot "src\apps\$app.c"
    $appRaw = Join-Path $buildDir "$app.raw"
    $appCmd = Join-Path $buildDir "$app.cmd"
    & $cl65 -t c128 -O -I $buildDir -C $appCfg -o $appRaw $appSource $appCrt0
    if ($LASTEXITCODE -ne 0) { throw "CC65 no pudo compilar la aplicacion $app." }

    & (Join-Path $projectRoot 'tools\make-cmd-program.ps1') `
        -InputFile $appRaw -OutputFile $appCmd -Bank BANK0
}

$editRaw = Join-Path $buildDir 'edit.raw'
$editCmd = Join-Path $buildDir 'edit.cmd'
& $cl65 -t c128 -O -C $bank1Cfg -o $editRaw (Join-Path $projectRoot 'src\apps\edit.c') $bank1Crt0
if ($LASTEXITCODE -ne 0) { throw 'CC65 no pudo compilar EDIT.' }
& (Join-Path $projectRoot 'tools\make-cmd-program.ps1') `
    -InputFile $editRaw -OutputFile $editCmd -Bank BANK1 -LoadAddress 0x1000

& $cl65 -t c128 -C (Join-Path $projectRoot 'cfg\cmdos-boot-sector.cfg') -o $bootSector `
    (Join-Path $projectRoot 'src\runtime\cmdos-boot-sector.s')
if ($LASTEXITCODE -ne 0) { throw 'CC65 no pudo compilar el sector de autoboot.' }

if (Test-Path $disk) { Remove-Item -LiteralPath $disk -Force }
$c1541 = Join-Path $viceBin 'c1541.exe'
& $c1541 -format 'cmd-os,co' d71 $disk `
    -write $program boot `
    -write (Join-Path $buildDir 'hello.cmd') hello.cmd `
    -write (Join-Path $buildDir 'mem.cmd') mem.cmd `
    -write (Join-Path $buildDir 'copy.cmd') copy.cmd `
    -write (Join-Path $buildDir 'help.cmd') help.cmd `
    -write $editCmd edit.cmd
if ($LASTEXITCODE -ne 0) { throw "VICE c1541 no pudo crear la imagen D71." }

& (Join-Path $projectRoot 'tools\make-boot-sector.ps1') -DiskImage $disk -BootSector $bootSector
if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar el sector de autoboot C128." }

Write-Host "Imagen creada: $disk"
Write-Host "Arranque de VICE: $viceBin\x128.exe -drive8type 1571 -8 $disk"
