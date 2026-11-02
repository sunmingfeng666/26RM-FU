# CLion calls this after building the selected CMake profile. No persistent server.
param(
    [Parameter(Mandatory=$true)][string]$Image,
    [Parameter(Mandatory=$true)][string]$Device,
    [Parameter(Mandatory=$true)][string]$TargetCfg,
    [ValidateSet('auto','daplink','jlink','stlink')][string]$Mode='auto', [string]$Serial='',
    [ValidateRange(1,50000)][int]$Speed=1000,
    [string]$OpenOcdRoot='',
    [string]$JLinkExe='C:\Program Files\SEGGER\JLink\JLink.exe', [string]$LogRoot='', [switch]$DryRun
)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'probe-common.ps1')
. (Join-Path $PSScriptRoot 'tool-runner.ps1')
try {
    if (-not $OpenOcdRoot) { $OpenOcdRoot = Split-Path (Split-Path (Get-Command openocd.exe -ErrorAction Stop).Source -Parent) -Parent }
    $probe = Select-Probe @(ConvertTo-ProbeCandidates @(Get-ProbeDevices -OpenOcdRoot $OpenOcdRoot)) $Mode $Serial
    $plan = New-FlashPlan $probe $Image $Device $OpenOcdRoot $TargetCfg $JLinkExe $Speed
    Write-Host "Probe: $($probe.Mode)  serial=$($probe.Serial)  usb=$($probe.UsbLocation)  driver=$($probe.Driver)  backend=$($plan.Backend)"
    if ($probe.SerialError) { Write-Host '[INFO] Serial descriptor unavailable; using the verified USB port binding.' }
    Write-Host "Image: $($plan.Image)"
    $sourceHash=Get-ImageSha256 $plan.Image
    Write-Host "SHA256: $sourceHash"
    if (-not $LogRoot) { $LogRoot = Join-Path $PSScriptRoot 'logs' }
    $run = Join-Path $LogRoot ((Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $run -Force | Out-Null
    # Freeze the image so a concurrent build cannot change bytes during download.
    $frozen = Join-Path $run ('firmware' + [IO.Path]::GetExtension($plan.Image))
    Copy-Item -LiteralPath $plan.Image -Destination $frozen
    if ((Get-ImageSha256 $frozen) -cne $sourceHash) { throw 'Image changed while freezing; rebuild and run again. No target was opened.' }
    $plan = New-FlashPlan $probe $frozen $Device $OpenOcdRoot $TargetCfg $JLinkExe $Speed $run
    $scriptName=if ($plan.Backend -eq 'segger') {'flash.jlink'} else {'flash.cfg'}
    $scriptFile = Join-Path $run $scriptName
    [IO.File]::WriteAllText($scriptFile,$plan.Script,(New-Object Text.UTF8Encoding $false))
    $toolArgs = @($plan.Arguments)
    if ($plan.Backend -eq 'segger') { $toolArgs += @('-CommandFile',$scriptName) } else { $toolArgs += @('-f',$scriptName) }
    $report = [ordered]@{ time=(Get-Date -Format o); probe=$probe; source=$Image; frozenImage=$frozen
        sha256=(Get-ImageSha256 $frozen); tool=$plan.Executable
        arguments=$toolArgs; workingDirectory=$run; imageArgument=$plan.ImageArgument
        dryRun=[bool]$DryRun; phase='image-check'; preflightExitCode=$null; exitCode=$null; success=$false }
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding UTF8
    Write-Host "Log: $run"
    if ($plan.Backend -eq 'openocd') {
        $report.preflightExitCode=Test-OpenOcdImage $plan.Executable $run $plan.ImageArgument
        $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding UTF8
        if ($report.preflightExitCode -ne 0) { throw "Image check failed (exit=$($report.preflightExitCode)). No physical target was opened. See $run" }
    }
    if ($DryRun) {
        $report.phase='dry-run-complete'
        $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding UTF8
        Write-Host '[DRY RUN] No physical target connection, reset or flash.'
        Write-Host "Tool: $($plan.Executable)"
        Write-Host "Working directory: $run"
        Write-Host ($toolArgs -join ' ')
        Write-Host $plan.Script
        exit 0
    }
    $report.phase='program'
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding UTF8
    $toolExit=Invoke-FlashTool $plan.Executable $toolArgs $run (Join-Path $run 'tool.log')
    $report.exitCode=$toolExit; $report.success=($toolExit -eq 0); $report.phase=if($report.success){'complete'}else{'failed'}
    $report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding UTF8
    if ($toolExit -ne 0) { throw "Flash/verify failed (exit=$toolExit). No retry was attempted. See $run" }
    Write-Host '[OK] Programming and verification completed; reset/run requested.'
    exit 0
} catch { [Console]::Error.WriteLine('[ERROR] ' + $_.Exception.Message); exit 1 }
