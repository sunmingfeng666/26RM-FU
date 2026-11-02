# Host-only regression checks. Never initializes or programs a target.
param([string]$WorkDir=(Join-Path $env:TEMP ('clion-flash-tests-' + [guid]::NewGuid().ToString('N'))),
      [string]$TargetCfg='target/stm32f4x.cfg', [string]$Device='STM32F407IG')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '..\probe-common.ps1')
$script:checks=0
function Assert($Condition, [string]$Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
    $script:checks++
}
function MustFail([scriptblock]$Action, [string]$Pattern) {
    $failed=$false
    try { & $Action | Out-Null } catch { $failed=$_.Exception.Message -match $Pattern }
    Assert $failed "Expected error: $Pattern"
}
function Device([string]$Id,[string]$Name,[string]$Driver,[string]$Class='USB',[string]$PhysicalId='',[string]$Status='OK') {
    if (-not $PhysicalId) { $PhysicalId=$Id }
    [pscustomobject]@{ InstanceId=$Id;PhysicalId=$PhysicalId;Name=$Name;ParentName='';Class=$Class;Status=$Status;Service=$Driver }
}
New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null
$root = Split-Path (Split-Path (Get-Command openocd.exe).Source -Parent) -Parent
$jlinkExe='C:\Program Files\SEGGER\JLink\JLink.exe'
$j=Device 'USB\VID_1366&PID_0105&MI_02\6&xxx&0&0002' 'J-Link driver' 'jlink' 'USB' 'USB\VID_1366&PID_0105\000602712450'
$cdc=Device 'USB\VID_1366&PID_0105&MI_00\6&xxx&0&0000' 'JLink CDC UART Port' 'JLinkCDC' 'Ports'
$parent=Device 'USB\VID_1366&PID_0105\000602712450' 'USB Composite Device' 'usbccgp'
$fake=Device 'USB\VID_0483&PID_5740\314E36483233' 'USB Serial Device' 'usbser' 'Ports'
$found=@(ConvertTo-ProbeCandidates @($j,$cdc,$parent,$fake))
Assert ($found.Count -eq 1) 'Composite J-Link counted once; UART excluded'
Assert ($found[0].Serial -eq '602712450') 'Physical numeric J-Link serial normalized'
Assert ($found[0].Backend -eq 'segger') 'Native SEGGER driver uses SEGGER backend'
Assert ((Select-Probe $found).Serial -eq '602712450') 'Unique probe auto-selected'
MustFail { Select-Probe @() } 'No matching'
MustFail { Select-Probe $found 'stlink' } 'No matching'
MustFail { Select-Probe $found 'auto' 'wrong' } 'No matching'
MustFail { New-ProbeConfig $found[0] $root $TargetCfg } 'SEGGER driver'
$s=Device 'USB\VID_0483&PID_3748\0123456789ABCDEF01234567' 'ST-LINK' 'WinUSB'
$sProbe=@(ConvertTo-ProbeCandidates @($s))[0]
Assert ($sProbe.Mode -eq 'stlink' -and $sProbe.Backend -eq 'openocd') 'ST-Link WinUSB'
MustFail { Select-Probe @($found[0],$sProbe) } 'Multiple probes'
Assert ((Select-Probe @($found[0],$sProbe) 'stlink').Mode -eq 'stlink') 'Explicit type selection'
Assert ((Select-Probe @($found[0],$sProbe) 'auto' '602712450').Mode -eq 'jlink') 'Explicit serial selection'
$d1=Device 'USB\VID_0D28&PID_0204&MI_00\6&xxx&0&0000' 'CMSIS-DAP' 'HidUsb' 'HIDClass' 'USB\VID_0D28&PID_0204\dap001'
$d2=Device 'USB\VID_0D28&PID_0204&MI_03\6&xxx&0&0003' 'CMSIS-DAP v2' 'WinUSB' 'USBDevice' 'USB\VID_0D28&PID_0204\dap001'
$dProbe=@(ConvertTo-ProbeCandidates @($d1,$d2))
Assert ($dProbe.Count -eq 1 -and $dProbe[0].Driver -eq 'WinUSB') 'DAP v1/v2 de-duplicated, prefer bulk interface'
Assert ((@(ConvertTo-ProbeCandidates @($d1)))[0].Backend -eq 'openocd') 'DAP v1 HID supported without driver replacement'
$genericHid=Device 'USB\VID_1234&PID_5678&MI_00\loc' 'USB Input Device' 'HidUsb' 'HIDClass' 'USB\VID_1234&PID_5678\dap003'
$genericHid.ParentName='USB Composite Device CMSIS-DAP'
Assert ((@(ConvertTo-ProbeCandidates @($genericHid)))[0].Mode -eq 'daplink') 'Generic HID name recognized from USB parent description'
$caseProbe=Device 'USB\VID_FAED&PID_4874&MI_00\location' 'Horco CMSIS-DAP v2' 'WinUSB' 'USBDevice' 'USB\VID_FAED&PID_4874\B7CC4C58'
$caseProbe | Add-Member NoteProperty UsbSerial 'b7cc4c58'
$caseProbe | Add-Member NoteProperty SerialError ''
$caseCandidate=@(ConvertTo-ProbeCandidates @($caseProbe))[0]
Assert ($caseCandidate.Serial -ceq 'b7cc4c58') 'Use original USB serial, not Windows uppercase instance ID'
Assert ($caseCandidate.WindowsSerial -ceq 'B7CC4C58') 'Keep Windows identity separately for diagnostics'
Assert ((Select-Probe @($caseCandidate) 'auto' 'B7CC4C58').Serial -ceq 'b7cc4c58') 'User selector can use Windows spelling, backend keeps original case'
Assert ((New-ProbeConfig $caseCandidate $root $TargetCfg) -cmatch 'adapter serial "b7cc4c58"') 'Exact serial emitted to OpenOCD'
$caseProbe.UsbSerial='MiXeD123'
Assert ((@(ConvertTo-ProbeCandidates @($caseProbe)))[0].Serial -ceq 'MiXeD123') 'Do not blindly lowercase arbitrary probe serials'
$caseProbe.SerialError='descriptor read failed'
MustFail { Select-Probe @(ConvertTo-ProbeCandidates @($caseProbe)) } 'original USB serial descriptor'
$d3=Device 'USB\VID_0D28&PID_0204\dap002' 'DAPLink' 'WinUSB'
MustFail { Select-Probe @(ConvertTo-ProbeCandidates @($d2,$d3)) } 'Multiple probes'
$bad=Device 'USB\VID_0483&PID_3748\bad' 'ST-LINK' '' 'USB' '' 'Error'
MustFail { Select-Probe @(ConvertTo-ProbeCandidates @($bad)) } 'Unsupported'
$jw=Device 'USB\VID_1366&PID_0105\000602712450' 'J-Link' 'WinUSB'
$jwProbe=@(ConvertTo-ProbeCandidates @($jw))[0]
Assert ($jwProbe.Backend -eq 'openocd') 'Already-WinUSB J-Link supported'
$fakeElf=Join-Path $WorkDir 'firmware [test] $dollar.elf'
[IO.File]::WriteAllBytes($fakeElf,[byte[]](0x7f,0x45,0x4c,0x46))
# Serial-less ST-Link: Windows' location instance suffix must never be emitted
# as a serial; the libusb bus+port binding must be proven unambiguous.
$legacy=Device 'USB\VID_0483&PID_3748\5&3207D635&0&10' 'STM32 STLink' 'WinUSB'
$legacy | Add-Member NoteProperty UsbSerial ''
$legacy | Add-Member NoteProperty SerialError 'Read USB string descriptor failed'
$legacy | Add-Member NoteProperty UsbLocation '2-10'
$legacy | Add-Member NoteProperty LocationError ''
$legacyProbe=@(ConvertTo-ProbeCandidates @($legacy))[0]
Assert ($legacyProbe.Backend -eq 'openocd' -and $legacyProbe.Serial -eq '') 'Serial read failure uses a verified ST-Link location'
Assert ($legacyProbe.WindowsSerial -eq '') 'Windows location suffix is not a fabricated serial'
Assert ((Select-Probe @($legacyProbe)).UsbLocation -eq '2-10') 'Single location-bound ST-Link can be selected'
$legacyCfg=New-ProbeConfig $legacyProbe $root $TargetCfg
Assert ($legacyCfg -match 'adapter usb location "2-10"' -and $legacyCfg -notmatch 'adapter serial') 'Generated script binds physical USB port only'
MustFail { Select-Probe @($legacyProbe,$sProbe) } 'Multiple probes'
MustFail { Select-Probe @($legacyProbe,$dProbe[0]) } 'Multiple probes'
MustFail { Select-Probe @($legacyProbe) 'auto' '5&3207D635&0&10' } 'No matching'
$libusbDevice=[pscustomobject]@{Vendor=0x0483;Product=0x3748;PortPath='10';Location='2-10'}
Assert ((Resolve-StLinkLocation $legacy.PhysicalId '10' @($libusbDevice)) -eq '2-10') 'PnP port chain and libusb VID/PID uniquely mapped'
$nested=[pscustomobject]@{Vendor=0x0483;Product=0x3748;PortPath='3.4';Location='2-3.4'}
Assert ((Resolve-StLinkLocation $legacy.PhysicalId '3.4' @($nested)) -eq '2-3.4') 'Nested hub port chain is preserved'
MustFail { Resolve-StLinkLocation $legacy.PhysicalId '11' @($libusbDevice) } 'uniquely map'
$otherBus=[pscustomobject]@{Vendor=0x0483;Product=0x3748;PortPath='10';Location='3-10'}
MustFail { Resolve-StLinkLocation $legacy.PhysicalId '10' @($libusbDevice,$otherBus) } 'uniquely map'
$wrongPid=[pscustomobject]@{Vendor=0x0483;Product=0x3754;PortPath='10';Location='2-10'}
MustFail { Resolve-StLinkLocation $legacy.PhysicalId '10' @($wrongPid) } 'uniquely map'
$legacy.UsbLocation=''
MustFail { Select-Probe @(ConvertTo-ProbeCandidates @($legacy)) } 'original USB serial descriptor'
$legacy.UsbLocation='not-a-location'
MustFail { Select-Probe @(ConvertTo-ProbeCandidates @($legacy)) } 'original USB serial descriptor'
$legacy.UsbLocation='2-10'; $legacy.Status='Error'
MustFail { Select-Probe @(ConvertTo-ProbeCandidates @($legacy)) } 'original USB serial descriptor'
# A DAPLink descriptor failure must still fail even if unrelated topology exists.
$caseProbe | Add-Member NoteProperty UsbLocation '2-10'
MustFail { Select-Probe @(ConvertTo-ProbeCandidates @($caseProbe)) } 'original USB serial descriptor'
$topologyFile=Join-Path $WorkDir 'stlink-location.cfg'
[IO.File]::WriteAllText($topologyFile,$legacyCfg,(New-Object Text.UTF8Encoding $false))
$ErrorActionPreference='Continue'
$topologyOutput=& (Join-Path $root 'bin\openocd.exe') -f $topologyFile -c 'shutdown' 2>&1
$topologyExit=$LASTEXITCODE
$ErrorActionPreference='Stop'
Assert ($topologyExit -eq 0) "USB location parsed by OpenOCD without target init: $topologyOutput"
$legacyPlan=New-FlashPlan $legacyProbe $fakeElf $Device $root $TargetCfg $jlinkExe
Assert ($legacyPlan.Script -match 'adapter usb location "2-10"' -and $legacyPlan.Script -match 'verify reset exit') 'Download keeps USB binding and verify-before-reset'
$plan=New-FlashPlan $found[0] $fakeElf $Device $root $TargetCfg $jlinkExe
Assert ($plan.Arguments -contains '602712450') 'SEGGER pins serial'
Assert ($plan.Script -match '(?s)^ExitOnError 1.*loadfile .*\nr\ng\nexit$') 'SEGGER errors stop before final reset/run'
MustFail { New-FlashPlan $found[0] $fakeElf '' $root $TargetCfg $jlinkExe } 'exact SEGGER'
MustFail { New-FlashPlan $found[0] 'missing.elf' $Device $root $TargetCfg $jlinkExe } 'Firmware file missing'
foreach ($p in @($sProbe,$dProbe[0],$jwProbe)) {
    $cfg=New-ProbeConfig $p $root $TargetCfg
    Assert ($cfg -match 'adapter serial') "$($p.Mode) pins serial"
    $file=Join-Path $WorkDir ($p.Mode+'.cfg')
    [IO.File]::WriteAllText($file,$cfg,(New-Object Text.UTF8Encoding $false))
    # shutdown before OpenOCD auto-init: parser check, no USB target connection.
    $ErrorActionPreference='Continue'
    $output=& (Join-Path $root 'bin\openocd.exe') -f $file -c 'shutdown' 2>&1
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    Assert ($code -eq 0) "$($p.Mode) config parsed by actual OpenOCD: $output"
    $fp=New-FlashPlan $p $fakeElf $Device $root $TargetCfg $jlinkExe
    Assert ($fp.Script -match 'verify reset exit') "$($p.Mode) verifies before reset/run"
    Assert ($fp.Script -match '\\\[test\\\] \\\$dollar') 'Tcl paths escaped'
}
Write-Host "PASS: $script:checks host checks. No hardware flash or reset."
