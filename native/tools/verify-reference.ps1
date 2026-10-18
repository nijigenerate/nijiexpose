param()
$ErrorActionPreference = 'Stop'
$nativeRepositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$nativeReferenceManifest = Get-Content -LiteralPath (Join-Path $nativeRepositoryRoot 'native/reference-sources.json') -Raw | ConvertFrom-Json
$nativeReferencePaths = @{}
foreach ($nativeReferenceFile in $nativeReferenceManifest.files) {
    $nativeReferencePaths[$nativeReferenceFile.path] = $true
    $nativeReferencePath = [System.IO.Path]::GetFullPath((Join-Path $nativeRepositoryRoot $nativeReferenceFile.path))
    if (-not $nativeReferencePath.StartsWith($nativeRepositoryRoot + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Reference path escapes the repository: $($nativeReferenceFile.path)"
    }
    if (-not (Test-Path -LiteralPath $nativeReferencePath -PathType Leaf)) {
        throw "Missing D reference: $($nativeReferenceFile.path)"
    }
    $nativeReferenceHash = (Get-FileHash -LiteralPath $nativeReferencePath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($nativeReferenceHash -ne $nativeReferenceFile.sha256) {
        throw "D reference changed; review its port mapping before updating the manifest: $($nativeReferenceFile.path)"
    }
}
# Use native PowerShell enumeration so verification does not require ripgrep on CI.
Get-ChildItem -LiteralPath (Join-Path $nativeRepositoryRoot 'source') -Recurse -File -Filter '*.d' | ForEach-Object {
    $nativeReferenceRelativePath = [System.IO.Path]::GetRelativePath($nativeRepositoryRoot, $_.FullName).Replace('\', '/')
    if ($nativeReferenceRelativePath -ne 'source/nijiexpose/ver.d' -and -not $nativeReferencePaths.ContainsKey($nativeReferenceRelativePath)) {
        throw "D source is not yet in the reference manifest: $nativeReferenceRelativePath"
    }
}
Write-Output "Verified $($nativeReferenceManifest.files.Count) D reference files. This checks reference integrity, not C++ implementation equivalence."
