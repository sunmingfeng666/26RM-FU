# Regression for the actual OpenOCD image reader under Windows PowerShell 5.1.
# No USB scan; only the built-in dummy adapter and simulated testee target run.
param([Parameter(Mandatory=$true)][string]$Image,
      [string]$WorkDir=(Join-Path $env:TEMP ('energy-image-test-'+[guid]::NewGuid().ToString('N'))))
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '..\probe-common.ps1')
. (Join-Path $PSScriptRoot '..\tool-runner.ps1')
$script:checks=0
function Assert($Value,[string]$Message) {
    if(-not $Value) { throw "FAIL: $Message" }
    $script:checks++
}
$Image=(Resolve-Path -LiteralPath $Image).Path
$hex=[IO.Path]::ChangeExtension($Image,'.hex')
if(-not (Test-Path -LiteralPath $hex)) { throw 'The built ELF and HEX are both required.' }
$ocd=(Get-Command openocd.exe -ErrorAction Stop).Source
$root=Split-Path (Split-Path $ocd -Parent) -Parent
# Build Chinese characters without depending on how PS5 decodes this script.
$unicode=([string][char]0x6D4B)+[char]0x8BD5+[char]0x76EE+[char]0x5F55
$base=Join-Path $WorkDir ($unicode+' [test] $dollar')
$probe=[pscustomobject]@{Mode='stlink';Serial='';UsbLocation='2-10';Backend='openocd';Driver='WinUSB'}
$segger=[pscustomobject]@{Mode='jlink';Serial='602712450';Backend='segger';Driver='jlink'}
$jlink='C:\Program Files\SEGGER\JLink\JLink.exe'
foreach($extension in @('elf','hex')) {
    $case=Join-Path $base $extension
    New-Item -ItemType Directory -Path $case -Force | Out-Null
    $source=if($extension -eq 'elf'){$Image}else{$hex}
    $frozen=Join-Path $case ('firmware.'+$extension)
    Copy-Item -LiteralPath $source -Destination $frozen
    $hash=Get-ImageSha256 $frozen
    Assert ($hash -ceq (Get-ImageSha256 $source)) 'Frozen image matches source hash'
    $plan=New-FlashPlan $probe $frozen 'STM32F103C8' $root 'target/stm32f1x.cfg' $jlink 1000 $case
    Assert ($plan.ImageArgument -ceq ('firmware.'+$extension)) 'Native image argument is an ASCII basename'
    Assert ($plan.WorkingDirectory -ceq (Resolve-Path -LiteralPath $case).Path) 'Unicode directory retained as Windows working directory'
    Assert ($plan.Script -notmatch '[^\x00-\x7f]') 'OpenOCD script does not embed the Chinese image path'
    Assert ($plan.Script -match ('program "firmware\.'+$extension+'" verify reset exit')) 'Programming and verification use the same frozen relative file'
    $code=Test-OpenOcdImage $ocd $case $plan.ImageArgument
    Assert ($code -eq 0) 'Real OpenOCD image parser accepts the image from a Chinese path'
    Assert ((Get-ImageSha256 $frozen) -ceq $hash) 'Host image check does not alter the frozen image'
    $checkScript=Get-Content -LiteralPath (Join-Path $case 'image-check.cfg') -Raw
    Assert ($checkScript -match '^adapter driver dummy' -and $checkScript -notmatch 'stlink|cmsis|load_image|program ') 'Image preflight cannot access a physical probe or program a target'
    $seggerPlan=New-FlashPlan $segger $frozen 'STM32F103C8' $root 'target/stm32f1x.cfg' $jlink 1000 $case
    Assert ($seggerPlan.Script -match ('loadfile "firmware\.'+$extension+'"')) 'J-Link also uses a relative frozen image'
}
$bad=Join-Path $base 'invalid'
New-Item -ItemType Directory -Path $bad -Force | Out-Null
[IO.File]::WriteAllBytes((Join-Path $bad 'firmware.elf'),[byte[]](0x7f,0x45,0x4c,0x46))
$code=Test-OpenOcdImage $ocd $bad 'firmware.elf'
Assert ($code -eq 1) 'Invalid ELF returns actual native exit code 1, not stale LASTEXITCODE -1'
$missing=Join-Path $base 'missing'
New-Item -ItemType Directory -Path $missing -Force | Out-Null
$code=Test-OpenOcdImage $ocd $missing 'firmware.elf'
Assert ($code -eq 1) 'Missing image is rejected by preflight before any physical target access'
Write-Host "PASS: $script:checks image path checks; Chinese directory + spaces + brackets + dollar. No hardware connection. Logs: $base"
