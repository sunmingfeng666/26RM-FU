# Shared discovery / command generation. Windows PowerShell 5.1 compatible.
Set-StrictMode -Version Latest

function Get-ImageSha256([string]$Path) {
    $stream=[IO.File]::OpenRead($Path)
    $hash=[Security.Cryptography.SHA256]::Create()
    try { [BitConverter]::ToString($hash.ComputeHash($stream)).Replace('-','') }
    finally { $hash.Dispose(); $stream.Dispose() }
}

function Get-ProbeDevices {
    param([string]$OpenOcdRoot='')
    # CM APIs read the parent directly. PnpDevice's bulk property query is slow /
    # inconsistent on this Windows PowerShell 5.1 host for composite interfaces.
    if (-not ('ClionFlash.Pnp' -as [type])) {
        Add-Type -Path (Join-Path $PSScriptRoot 'windows-usb.cs')
    }
    # CIM exposes Service in the same query; avoid one slow WMI call per USB node.
    $devices = @(Get-CimInstance Win32_PnPEntity -Filter 'Present = True' -ErrorAction Stop | Where-Object PNPDeviceID -Like 'USB\*')
    $byId = @{}
    $usbSerials = @{}
    $serialErrors = @{}
    $locations = @{}
    $locationErrors = @{}
    foreach ($d in $devices) { $byId[$d.PNPDeviceID] = $d }
    foreach ($d in $devices) {
        if ($d.PNPClass -in @('Ports','Net','DiskDrive','Volume')) { continue }
        $service = [string]$d.Service
        if ($service -match '^(usbccgp|usbser|USBSTOR|usbhub.*|usbvideo|usbaudio.*)$') { continue }
        $parentId = [ClionFlash.Pnp]::Parent($d.PNPDeviceID)
        $parentName = if ($parentId -and $byId.ContainsKey($parentId)) { [string]$byId[$parentId].Name } else { '' }
        $parentName += ' ' + [ClionFlash.Pnp]::Description($parentId)
        $physicalId = $d.PNPDeviceID
        if ($parentId -match '^USB\\VID_' -and $d.PNPDeviceID -match '&MI_') { $physicalId = $parentId }
        $name = ([string]$d.Name + ' ' + [ClionFlash.Pnp]::Description($d.PNPDeviceID)).Trim()
        $kind = Get-ProbeMode $d.PNPDeviceID ($name + ' ' + $parentName)
        if (-not $kind) { continue }
        if (-not $usbSerials.ContainsKey($physicalId)) {
            try { $usbSerials[$physicalId] = [ClionFlash.Pnp]::Serial($physicalId) }
            catch { $usbSerials[$physicalId] = ''; $serialErrors[$physicalId] = $_.Exception.Message }
            if ($kind -eq 'stlink' -and (-not $usbSerials[$physicalId] -or $serialErrors[$physicalId])) {
                try {
                    if (-not $OpenOcdRoot) { $OpenOcdRoot=Split-Path (Split-Path (Get-Command openocd.exe -ErrorAction Stop).Source -Parent) -Parent }
                    if (-not ('ClionFlash.UsbLocations' -as [type])) { Add-Type -Path (Join-Path $PSScriptRoot 'usb-locations.cs') }
                    $portPath=[ClionFlash.Pnp]::PortPath($physicalId)
                    $usbDevices=@([ClionFlash.UsbLocations]::Read((Join-Path $OpenOcdRoot 'bin\libusb-1.0.dll')))
                    $locations[$physicalId]=Resolve-StLinkLocation $physicalId $portPath $usbDevices
                } catch { $locationErrors[$physicalId]=$_.Exception.GetBaseException().Message }
            }
        }
        [pscustomobject]@{ InstanceId=[string]$d.PNPDeviceID; PhysicalId=$physicalId; Name=$name
            ParentName=$parentName; Class=[string]$d.PNPClass; Status=[string]$d.Status; Service=$service
            UsbSerial=$usbSerials[$physicalId]; SerialError=$serialErrors[$physicalId]
            UsbLocation=$locations[$physicalId]; LocationError=$locationErrors[$physicalId] }
    }
}

function Get-ProbeMode([string]$Id, [string]$Description) {
    if ($Id -match '^USB\\VID_1366&') { return 'jlink' }
    if ($Id -match '^USB\\VID_0483&PID_(3744|3748|374B|374D|374E|374F|3752|3753|3754|3755|3757)(?:&|\\)') { return 'stlink' }
    if ($Description -match 'CMSIS[- _]?DAP|DAP[- _]?Link') { return 'daplink' }
    return ''
}

function Resolve-StLinkLocation([string]$PhysicalId, [string]$PortPath, [object[]]$UsbDevices) {
    if ($PhysicalId -notmatch '^USB\\VID_0483&PID_([0-9A-F]{4})(?:&|\\)') { throw 'Invalid ST-Link identity' }
    $product=[Convert]::ToUInt16($Matches[1],16)
    if ($PortPath -notmatch '^\d+(\.\d+)*$') { throw 'Invalid USB port path' }
    $matched=@($UsbDevices | Where-Object { $_.Vendor -eq 0x0483 -and $_.Product -eq $product -and $_.PortPath -ceq $PortPath })
    if ($matched.Count -ne 1) { throw "Cannot uniquely map ST-Link to libusb topology ($($matched.Count) matches); no device will be selected." }
    if ($matched[0].Location -notmatch '^\d+-\d+(\.\d+)*$') { throw 'Invalid libusb location' }
    return [string]$matched[0].Location
}

function ConvertTo-ProbeCandidates {
    param([object[]]$Devices)
    $seen = @{}
    foreach ($d in $Devices) {
        if ($d.InstanceId -notmatch '^USB\\' -or $d.Class -in @('Ports','Net','DiskDrive','Volume')) { continue }
        if ($d.Service -match '^(usbccgp|usbser|USBSTOR|usbhub.*|usbvideo|usbaudio.*)$') { continue }
        $kind = Get-ProbeMode $d.InstanceId ($d.Name + ' ' + $d.ParentName)
        if (-not $kind) { continue }
        $serial = ($d.PhysicalId -split '\\')[-1]
        if ($serial -match '&') { $serial = '' } # Windows location suffix, not a USB serial.
        $windowsSerial = $serial
        # Windows folds instance IDs to uppercase. OpenOCD compares raw USB strings
        # case-sensitively, so never lowercase/uppercase a USB descriptor by guess.
        if ($d.PSObject.Properties['UsbSerial']) { $serial = [string]$d.UsbSerial }
        if ($kind -eq 'jlink' -and $serial -match '^\d+$') { $serial = ([uint64]$serial).ToString() }
        $backend = 'unsupported'
        $reason = "Unsupported debug-interface driver '$($d.Service)' (status $($d.Status))."
        if ($d.Status -eq 'OK') {
            if ($d.Service -match '^(WinUSB|libusbK|libusb0)$') { $backend = 'openocd'; $reason = '' }
            elseif ($kind -eq 'daplink' -and $d.Service -eq 'HidUsb') { $backend = 'openocd'; $reason = '' }
            elseif ($kind -eq 'jlink' -and $d.Service -eq 'jlink') { $backend = 'segger'; $reason = '' }
        }
        $serialError=if ($d.PSObject.Properties['SerialError']) { [string]$d.SerialError } else { '' }
        $location=if ($d.PSObject.Properties['UsbLocation']) { [string]$d.UsbLocation } else { '' }
        if ($serialError) {
            if ($kind -eq 'stlink' -and $backend -eq 'openocd' -and $location -match '^\d+-\d+(\.\d+)*$') {
                $serial=''; # Keep descriptor failure visible, bind the verified physical USB port instead.
            } else {
                $backend='unsupported'; $reason='Cannot read the original USB serial descriptor: ' + $serialError
                if ($d.PSObject.Properties['LocationError'] -and $d.LocationError) { $reason+='; USB location: ' + $d.LocationError }
            }
        }
        if ($kind -eq 'stlink' -and -not $serial -and $location -notmatch '^\d+-\d+(\.\d+)*$') {
            $backend='unsupported'; if (-not $reason) { $reason='ST-Link has neither a readable serial nor a verified USB location.' }
        }
        $candidate = [pscustomobject]@{ Mode=$kind; Serial=$serial; WindowsSerial=$windowsSerial; Backend=$backend; Driver=$d.Service
            Status=$d.Status; Name=$d.Name; InstanceId=$d.InstanceId; PhysicalId=$d.PhysicalId; Reason=$reason
            UsbLocation=$location; SerialError=$serialError }
        $key = $kind + ':' + $d.PhysicalId
        # DAP v1 + v2 interfaces of one probe represent one physical target.
        if (-not $seen.ContainsKey($key) -or ($seen[$key].Backend -eq 'unsupported' -and $backend -ne 'unsupported') -or
            ($seen[$key].Driver -eq 'HidUsb' -and $d.Service -eq 'WinUSB')) { $seen[$key] = $candidate }
    }
    @($seen.Values | Sort-Object Mode,Serial,InstanceId)
}

function Select-Probe {
    param([object[]]$Candidates, [string]$Mode='auto', [string]$Serial='')
    $matches = @($Candidates | Where-Object { ($Mode -eq 'auto' -or $_.Mode -eq $Mode) -and (-not $Serial -or $_.Serial -eq $Serial) })
    if ($matches.Count -eq 0) { throw "No matching connected debug probe (mode=$Mode serial=$Serial). Check USB/driver; serial ports do not count as probes." }
    if ($matches.Count -gt 1) {
        $list = ($matches | ForEach-Object { "$($_.Mode):$($_.Serial) usb=$($_.UsbLocation) [$($_.Driver)]" }) -join ', '
        throw "Multiple probes: $list. Select one with -Mode and/or -Serial; no target was touched."
    }
    if ($matches[0].Backend -eq 'unsupported') { throw "$($matches[0].Mode) $($matches[0].InstanceId): $($matches[0].Reason)" }
    return $matches[0]
}

function ConvertTo-TclString {
    param([string]$Value)
    '"' + $Value.Replace('\','/').Replace('"','\"').Replace('$','\$').Replace('[','\[').Replace(']','\]') + '"'
}

function New-ProbeConfig {
    param($Probe, [string]$OpenOcdRoot, [string]$TargetCfg, [int]$Speed=1000)
    if ($Probe.Backend -ne 'openocd') { throw 'This J-Link uses the SEGGER driver. Use auto-flash.ps1 / the updated CLion Flash configuration; raw OpenOCD cannot open this driver.' }
    $scripts = Join-Path $OpenOcdRoot 'share\openocd\scripts'
    if (-not (Test-Path -LiteralPath $scripts -PathType Container)) {
        $scripts = Join-Path $OpenOcdRoot 'openocd\scripts' # xPack distribution
    }
    $iface = switch ($Probe.Mode) { 'daplink' {'cmsis-dap'} 'jlink' {'jlink'} 'stlink' {'stlink'} default {throw 'Unknown probe.'} }
    $transport = if ($Probe.Mode -eq 'stlink') {
        'if { [adapter name] eq "hla" } { transport select hla_swd } else { transport select swd }'
    } else { 'transport select swd' }
    $interfaceFile = Join-Path $scripts "interface\$iface.cfg"
    $targetFile = Join-Path $scripts $TargetCfg
    foreach ($path in @($interfaceFile,$targetFile)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "OpenOCD config missing: $path" }
    }
    $lines = @("# Selected $($Probe.Mode), driver=$($Probe.Driver)","add_script_search_dir $(ConvertTo-TclString $scripts)",
        "source $(ConvertTo-TclString $interfaceFile)", $transport)
    if ($Probe.Serial) { $lines += "adapter serial $(ConvertTo-TclString $Probe.Serial)" }
    elseif ($Probe.Mode -eq 'stlink' -and $Probe.UsbLocation -match '^\d+-\d+(\.\d+)*$') {
        $lines += "adapter usb location $(ConvertTo-TclString $Probe.UsbLocation)"
    }
    elseif ($Probe.Mode -eq 'stlink') { throw 'ST-Link requires a serial or a verified USB location.' }
    $lines += @("source $(ConvertTo-TclString $targetFile)", 'reset_config none', "adapter speed $Speed")
    $lines -join "`n"
}

function New-FlashPlan {
    param($Probe, [string]$Image, [string]$Device, [string]$OpenOcdRoot,
        [string]$TargetCfg, [string]$JLinkExe, [int]$Speed=1000, [string]$WorkingDirectory='')
    if (-not (Test-Path -LiteralPath $Image -PathType Leaf)) { throw "Firmware file missing: $Image. Build the selected CMake profile first." }
    $Image = (Resolve-Path -LiteralPath $Image).Path
    if ([IO.Path]::GetExtension($Image) -notin @('.elf','.hex')) { throw 'Use ELF/HEX with embedded addresses. Raw BIN is not accepted.' }
    if ($Image -match '[\r\n"]') { throw 'Unsupported firmware filename.' }
    $imageArgument=$Image
    if ($WorkingDirectory) {
        $WorkingDirectory=(Resolve-Path -LiteralPath $WorkingDirectory).Path
        if ((Split-Path $Image -Parent) -ine $WorkingDirectory) { throw 'Frozen image must be inside the tool working directory.' }
        $imageArgument=[IO.Path]::GetFileName($Image)
        if ($imageArgument -notmatch '^firmware\.(elf|hex)$') { throw 'Frozen image must use a fixed ASCII filename.' }
    }
    if ($Probe.Backend -eq 'segger') {
        if (-not $Device -or $Device -notmatch '^[a-zA-Z0-9_+.-]+$') { throw 'Supply the exact SEGGER -Device name for this MCU.' }
        if (-not $Probe.Serial -or $Probe.Serial -notmatch '^\d+$') { throw 'Cannot determine a unique numeric J-Link serial.' }
        if (-not (Test-Path -LiteralPath $JLinkExe -PathType Leaf)) { throw "SEGGER JLink.exe missing: $JLinkExe" }
        # LoadFile verifies by default; ExitOnError stops before reset/go on failure.
        $script = @('ExitOnError 1','r','h',"loadfile `"$($imageArgument.Replace('\','/'))`"",'r','g','exit') -join "`n"
        return [pscustomobject]@{ Backend='segger'; Executable=$JLinkExe; Image=$Image; Script=$script; ImageArgument=$imageArgument; WorkingDirectory=$WorkingDirectory
            Arguments=@('-USB',$Probe.Serial,'-Device',$Device,'-If','SWD','-Speed',"$Speed",'-AutoConnect','1','-NoGui','1','-ExitOnError','1') }
    }
    $exe = Join-Path $OpenOcdRoot 'bin\openocd.exe'
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "OpenOCD missing: $exe" }
    $script = New-ProbeConfig $Probe $OpenOcdRoot $TargetCfg $Speed
    $script += "`ngdb port disabled`ntcl port disabled`ntelnet port disabled`nprogram $(ConvertTo-TclString $imageArgument) verify reset exit`n"
    [pscustomobject]@{ Backend='openocd'; Executable=$exe; Image=$Image; Script=$script; Arguments=@(); ImageArgument=$imageArgument; WorkingDirectory=$WorkingDirectory }
}
