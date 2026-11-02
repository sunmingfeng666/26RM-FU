# CLion builds its selected profile before running this entry point.
param(
    [Parameter(Mandatory=$true)][string]$BuildDir,
    [Parameter(Mandatory=$true)][ValidateSet('master','slave1','slave2','slave3','slave4','slave5','slave6','slave7','motor')][string]$ExpectedBoard,
    [ValidateSet('','Debug','Release')][string]$ExpectedConfiguration='',
    [ValidateSet('auto','daplink','jlink','stlink')][string]$Mode='auto',
    [string]$Serial='', [string]$OpenOcdRoot='',
    [string]$JLinkExe='C:\Program Files\SEGGER\JLink\JLink.exe',
    [switch]$DryRun, [switch]$PlanOnly
)
$ErrorActionPreference='Stop'
try {
    $build=(Resolve-Path -LiteralPath $BuildDir).Path
    $manifest=Get-Content -LiteralPath (Join-Path $build 'firmware.json') -Raw | ConvertFrom-Json
    if ($manifest.schema -ne 1 -or $manifest.board -cne $ExpectedBoard -or $manifest.image -cne 'EnergyFirmware.elf') {
        throw "Build/profile identity mismatch: requested=$ExpectedBoard, built=$($manifest.board), directory=$build. Select STM32-$ExpectedBoard-Debug (or Release) in CLion; no probe was opened."
    }
    if ($ExpectedConfiguration -and $manifest.buildType -cne $ExpectedConfiguration) {
        throw "Build type mismatch: requested=$ExpectedConfiguration, built=$($manifest.buildType). No probe was opened."
    }
    $device=if ($ExpectedBoard -eq 'master') {'STM32F407IG'} else {'STM32F103C8'}
    $target=if ($ExpectedBoard -eq 'master') {'target/stm32f4x.cfg'} else {'target/stm32f1x.cfg'}
    if ($manifest.device -cne $device -or $manifest.targetCfg -cne $target) { throw 'Unexpected MCU/target in firmware manifest.' }
    $image=Join-Path $build 'EnergyFirmware.elf'
    if (-not (Test-Path -LiteralPath $image -PathType Leaf)) { throw 'Build the selected profile first.' }
    $local=Join-Path $PSScriptRoot 'flash.local.json'
    if (Test-Path -LiteralPath $local) {
        $settings=Get-Content -LiteralPath $local -Raw | ConvertFrom-Json
        if (-not $OpenOcdRoot -and $settings.openOcdRoot) { $OpenOcdRoot=$settings.openOcdRoot }
        if ($settings.jLinkExe) { $JLinkExe=$settings.jLinkExe }
        if (-not $Serial -and $settings.boards.$ExpectedBoard) {
            $Serial=[string]$settings.boards.$ExpectedBoard.serial
            $Mode=[string]$settings.boards.$ExpectedBoard.mode
        }
    }
    if (-not $OpenOcdRoot) {
        $command=Get-Command openocd.exe -ErrorAction SilentlyContinue
        if ($command) { $OpenOcdRoot=Split-Path (Split-Path $command.Source -Parent) -Parent }
    }
    if ($PlanOnly) {
        [pscustomobject]@{board=$ExpectedBoard; device=$device; target=$target; image=$image; mode=$Mode; serial=$Serial; openOcdRoot=$OpenOcdRoot} | ConvertTo-Json
        Write-Host '[PLAN ONLY] No USB scan, connection, reset or flash.'
        exit 0
    }
    & (Join-Path $PSScriptRoot 'auto-flash.ps1') -Image $image -Device $device -TargetCfg $target -Mode $Mode -Serial $Serial -OpenOcdRoot $OpenOcdRoot -JLinkExe $JLinkExe -LogRoot (Join-Path $PSScriptRoot 'logs') -DryRun:$DryRun
    exit $LASTEXITCODE
} catch { [Console]::Error.WriteLine('[ERROR] ' + $_.Exception.Message); exit 1 }
