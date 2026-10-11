$ErrorActionPreference = 'Stop'
$testRoot = Join-Path $env:RUNNER_TEMP 'nijiexpose-logging-test'
New-Item -ItemType Directory -Force -Path $testRoot | Out-Null
$executable = Join-Path $testRoot 'logging-smoke.exe'
& ldc2 -mscrtlib=msvcrt -Isource build-aux/windows/logging-smoke.d source/nijiexpose/log.d "-of=$executable" -L/SUBSYSTEM:WINDOWS -L/ENTRY:wmainCRTStartup
if ($LASTEXITCODE -ne 0) { throw 'Logging smoke test compilation failed' }
$image = [IO.File]::ReadAllBytes($executable)
$peOffset = [BitConverter]::ToInt32($image, 0x3c)
if ([BitConverter]::ToUInt16($image, $peOffset + 24 + 68) -ne 2) {
    throw 'Logging test must use the Windows GUI subsystem'
}
$originalLocalAppData = $env:LOCALAPPDATA
try {
    $env:LOCALAPPDATA = Join-Path $testRoot '日本語'
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw 'Console-less logging failed' }
    $logPath = Join-Path $env:LOCALAPPDATA 'nijiexpose/logs/nijiexpose.log'
    $log = Get-Content -LiteralPath $logPath -Raw
    foreach ($expected in @('logging smoke test: 100%', 'direct stdout smoke test', 'direct stderr smoke test', 'direct C stdout smoke test')) {
        if (-not $log.Contains($expected)) { throw "Missing log output: $expected" }
    }
    [IO.File]::WriteAllText($logPath, ('x' * (5 * 1024 * 1024)))
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0 -or -not (Test-Path -LiteralPath "$logPath.previous")) {
        throw 'Log rotation failed'
    }
    $blockedRoot = Join-Path $testRoot 'blocked'
    Set-Content -LiteralPath $blockedRoot -Value 'This is a file, not a directory.'
    $env:LOCALAPPDATA = $blockedRoot
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw 'Unavailable log directory fallback failed' }
    $env:LOCALAPPDATA = Join-Path $testRoot 'unopenable-log'
    New-Item -ItemType Directory -Force (Join-Path $env:LOCALAPPDATA 'nijiexpose/logs/nijiexpose.log') | Out-Null
    $process = Start-Process -FilePath $executable -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw 'Unopenable log file fallback failed' }
} finally {
    $env:LOCALAPPDATA = $originalLocalAppData
}
