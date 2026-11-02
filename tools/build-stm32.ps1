param([ValidateSet('all','master','slave1','slave2','slave3','slave4','slave5','slave6','slave7','motor')][string]$Board='master',
      [ValidateSet('Debug','Release')][string]$Configuration='Debug', [string]$Ninja='', [switch]$Clean)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(-not $Ninja) {
    $localNinja=Join-Path $root '.tools\venv\Scripts\ninja.exe'
    if(Test-Path -LiteralPath $localNinja) { $Ninja=$localNinja }
    else { $command=Get-Command ninja.exe -ErrorAction SilentlyContinue; if($command) { $Ninja=$command.Source } }
}
if(-not $Ninja) { throw 'Specify -Ninja <path to CLion bundled ninja.exe>, or put Ninja on PATH.' }
$boards=if($Board -eq 'all') {@('master','slave1','slave2','slave3','slave4','slave5','slave6','slave7','motor')} else {@($Board)}
Push-Location $root
try {
    foreach($item in $boards) {
        $preset=$item+'-'+$Configuration
        & cmake --preset $preset ('-DCMAKE_MAKE_PROGRAM='+$Ninja.Replace('\','/'))
        if($LASTEXITCODE) { throw "Configure failed: $preset" }
        $argsList=@('--build','--preset',$preset,'-j','8')
        if($Clean) { $argsList+='--clean-first' }
        & cmake @argsList
        if($LASTEXITCODE) { throw "Build failed: $preset" }
    }
} finally { Pop-Location }
