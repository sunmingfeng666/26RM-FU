# Native execution with a Unicode Windows working directory and ASCII file args.
# Avoids PowerShell 5.1 pipeline scoping of LASTEXITCODE; logs the real exit code.
Set-StrictMode -Version Latest

function ConvertTo-NativeArgument([string]$Value) {
    if ($Value -notmatch '[\s"]' -and $Value.Length) { return $Value }
    $escaped=[regex]::Replace($Value,'(\\*)"','$1$1\"')
    $escaped=[regex]::Replace($escaped,'(\\+)$','$1$1')
    return '"'+$escaped+'"'
}

function Invoke-FlashTool {
    param([string]$Executable, [string[]]$Arguments, [string]$WorkingDirectory, [string]$LogFile)
    $start=New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName=$Executable
    $start.Arguments=(@($Arguments | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' ')
    $start.WorkingDirectory=(Resolve-Path -LiteralPath $WorkingDirectory).Path
    $start.UseShellExecute=$false
    $start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true
    $start.RedirectStandardError=$true
    $process=New-Object System.Diagnostics.Process
    $process.StartInfo=$start
    try {
        if (-not $process.Start()) { throw 'Could not start the programming tool.' }
        # Read both streams concurrently so neither pipe can fill and deadlock.
        $stdout=$process.StandardOutput.ReadToEndAsync()
        $stderr=$process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $exitCode=$process.ExitCode
        $output=$stdout.GetAwaiter().GetResult()
        $errors=$stderr.GetAwaiter().GetResult()
        $log="[stdout]`r`n$output`r`n[stderr]`r`n$errors"
        [IO.File]::WriteAllText($LogFile,$log,(New-Object Text.UTF8Encoding $false))
        if ($output) { Write-Host $output.TrimEnd() }
        if ($errors) { Write-Host $errors.TrimEnd() }
        return [int]$exitCode
    } finally { $process.Dispose() }
}

function Test-OpenOcdImage {
    param([string]$Executable,[string]$WorkingDirectory,[string]$ImageName)
    if ($ImageName -notmatch '^firmware\.(elf|hex)$') { throw 'Image preflight requires a frozen ASCII filename.' }
    $format=if($ImageName.EndsWith('.elf')){'elf'}else{'ihex'}
    # A dummy adapter and testee target live only in the host process. They make
    # OpenOCD's real image parser available WITHOUT USB/SWD or a physical target.
    # The fake chain has no electrical scan; no real interface/target cfg is read.
    $script=@('adapter driver dummy','adapter speed 1000','transport select jtag',
        'jtag newtap imagecheck cpu -irlen 4',
        'target create imagecheck.cpu testee -chain-position imagecheck.cpu',
        'gdb port disabled','tcl port disabled','telnet port disabled',
        'proc jtag_init {} {}','init',"echo [test_image $ImageName 0 $format]",'shutdown') -join "`n"
    [IO.File]::WriteAllText((Join-Path $WorkingDirectory 'image-check.cfg'),$script,(New-Object Text.UTF8Encoding $false))
    Write-Host '[HOST IMAGE CHECK] Dummy adapter only; no physical target connection.'
    return Invoke-FlashTool $Executable @('-f','image-check.cfg') $WorkingDirectory (Join-Path $WorkingDirectory 'image-check.log')
}
