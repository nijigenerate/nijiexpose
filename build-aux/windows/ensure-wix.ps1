$ErrorActionPreference = 'Stop'

function Find-Wix3 {
    $roots = @($env:WIX, "${env:ProgramFiles(x86)}\WiX Toolset v3.14")
    foreach ($root in $roots) {
        if (-not $root) { continue }
        $candle = Join-Path $root 'bin/candle.exe'
        $light = Join-Path $root 'bin/light.exe'
        if ((Test-Path -LiteralPath $candle) -and
            (Test-Path -LiteralPath $light)) {
            return $root
        }
    }
}

# Hosted Windows runners already include WiX 3.14. Do not attempt to
# downgrade a newer Chocolatey package revision of the same toolset.
$wixRoot = Find-Wix3
if (-not $wixRoot) {
    choco install wixtoolset --version=3.14.1.20250415 --yes --no-progress
    if ($LASTEXITCODE -ne 0) { throw 'WiX installation failed' }
    $wixRoot = Find-Wix3
    if (-not $wixRoot) { throw 'WiX v3 tools are missing' }
}
Write-Output "Using installed WiX v3: $wixRoot"

$env:PATH = (Join-Path $wixRoot "bin") + [IO.Path]::PathSeparator + $env:PATH
if ($env:GITHUB_PATH) { Join-Path $wixRoot "bin" | Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8 }
