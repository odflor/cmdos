param(
    [string] $Cc65Bin = 'D:\cc65\bin',
    [string] $ViceBin = 'D:\GTK3VICE-3.10-win64\bin'
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
$disk = Join-Path $buildDir 'cmd-os.d64'

New-Item -ItemType Directory -Force $buildDir | Out-Null

$cl65 = Join-Path $Cc65Bin 'cl65.exe'
& $cl65 -t c128 -O -m $map -o $program $source `
    (Join-Path $projectRoot 'src\kernel\tui.c') `
    (Join-Path $projectRoot 'src\kernel\bank1-gate.s')
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
        if ($endAddress -ge 0x5000) {
            throw "El segmento $name de CMD-OS invade la zona reservada para aplicaciones."
        }
        $residentEnd = [Math]::Max($residentEnd, $endAddress)
        '    {{ "{0}", 0x{1}, 0x{2}, 0 }},' -f $name, $start, $end
    }
}
if ($residentEnd -lt 0x4FFF) {
    $memoryRows += '    {{ "RAM libre para CMD-OS", 0x{0:X4}, 0x4FFF, 1 }},' -f ($residentEnd + 1)
}
$memoryRows += '    { "Area CMDP (reserva)", 0x5000, 0xAFFF, 0 },',
    '    { "Pila C CMDP (reserva)", 0xB000, 0xB7FF, 0 },',
    '    { "Pila C CMD-OS (reserva)", 0xB800, 0xBFFF, 0 },'
Set-Content -LiteralPath $memoryInclude -Value $memoryRows -Encoding Ascii

foreach ($app in 'hello', 'mem') {
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
    -InputFile $editRaw -OutputFile $editCmd -Bank BANK1 -LoadAddress 0x1000 -ApiVersion 2

if (Test-Path $disk) { Remove-Item -LiteralPath $disk -Force }
$c1541 = Join-Path $viceBin 'c1541.exe'
& $c1541 -format 'cmd-os,co' d64 $disk `
    -write $program boot `
    -write (Join-Path $buildDir 'hello.cmd') hello.cmd `
    -write (Join-Path $buildDir 'mem.cmd') mem.cmd `
    -write $editCmd edit.cmd
if ($LASTEXITCODE -ne 0) { throw "VICE c1541 no pudo crear la imagen D64." }

& (Join-Path $projectRoot 'tools\make-boot-sector.ps1') -DiskImage $disk -Program $program
if ($LASTEXITCODE -ne 0) { throw "No se pudo instalar el sector de autoboot C128." }

Write-Host "Imagen creada: $disk"
Write-Host "Arranque de VICE: $viceBin\x128.exe -8 $disk"
