param([Parameter(Mandatory)][string]$Source)
$ErrorActionPreference = 'Stop'
$file = Join-Path $Source 'native/scripts/build_mediapipe_halide.ps1'
$text = [IO.File]::ReadAllText($file)
if ($text.Contains('# nijiexpose Bazel cache')) { return }
if ((Get-FileHash $file -Algorithm SHA256).Hash -ne '8D153CEC5BF2DD153C2BB5FEBA4425100287CEC06669C33CDE62307A6C8FCA06') {
    throw 'Unknown nijitrack Windows build script; rebase the cache integration.'
}
$anchor = 'Push-Location $config.source'
$insert = @'
# nijiexpose Bazel cache: cache compiler outputs, never substitute a downloaded SDK.
if ($env:NIJIEXPOSE_BAZEL_CACHE) {
  $cache = $env:NIJIEXPOSE_BAZEL_CACHE.Replace('\','/')
  New-Item -ItemType Directory -Force "$cache/actions" | Out-Null
  $argsList += "--disk_cache=$cache/actions"
}
'@
if (!$text.Contains($anchor)) { throw 'Missing upstream build anchor' }
[IO.File]::WriteAllText($file, $text.Replace($anchor, "$insert`n$anchor"), [Text.UTF8Encoding]::new($false))
