param([Parameter(Mandatory)][string]$Source)
$ErrorActionPreference = 'Stop'
$file = Join-Path $Source 'native/scripts/build_mediapipe_halide.ps1'
$text = [IO.File]::ReadAllText($file)
if ($text.Contains('# nijiexpose initialize bundled Java')) { return }
$hash = (Get-FileHash $file -Algorithm SHA256).Hash
if ($hash -notin @('8D153CEC5BF2DD153C2BB5FEBA4425100287CEC06669C33CDE62307A6C8FCA06',
    '7593D2395D364A8CEC0C56105BFB720DA7C79FD7A27A703B3C1E8B0F19308D86')) {
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
if (!$text.Contains('# nijiexpose Bazel cache')) { $text = $text.Replace($anchor, "$insert`n$anchor") }
$bootstrap = @'
# nijiexpose initialize bundled Java before upstream discovers it on a fresh build.
& $trackingEnvironment.bazel --nosystem_rc --nohome_rc --noworkspace_rc --batch "--output_user_root=$root" version
if ($LASTEXITCODE) { throw 'Bazel initialization failed' }
'@
$text = $text.Replace('$javaName =', "$bootstrap`n`$javaName =")
[IO.File]::WriteAllText($file, $text, [Text.UTF8Encoding]::new($false))
