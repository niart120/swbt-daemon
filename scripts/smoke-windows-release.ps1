param(
    [string]$PackageDir = ""
)

$ErrorActionPreference = "Stop"

if (-not $PackageDir) {
    $candidate = Get-ChildItem -Path "dist" -Directory -Filter "swbt-daemon-v*-windows-x86_64" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $candidate) {
        throw "No package directory found under dist"
    }
    $PackageDir = $candidate.FullName
}

$resolvedPackageDir = Resolve-Path -LiteralPath $PackageDir
$daemon = Join-Path $resolvedPackageDir "bin\swbt-daemon.exe"
$debugClient = Join-Path $resolvedPackageDir "bin\swbt-debug-client.exe"

foreach ($path in @(
        $daemon,
        $debugClient,
        (Join-Path $resolvedPackageDir "README.md"),
        (Join-Path $resolvedPackageDir "LICENSE"),
        (Join-Path $resolvedPackageDir "THIRD_PARTY_NOTICES.md"),
        (Join-Path $resolvedPackageDir "licenses\btstack\LICENSE"),
        (Join-Path $resolvedPackageDir "licenses\btstack\3rd-party\README.md"),
        (Join-Path $resolvedPackageDir "licenses\toml11\LICENSE"),
        (Join-Path $resolvedPackageDir "manifest.json")
    )) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing package file: $path"
    }
}

function Invoke-ExpectExit {
    param(
        [string]$Path,
        [string[]]$Arguments,
        [int]$ExpectedExitCode
    )

    & $Path @Arguments
    $actual = $LASTEXITCODE
    if ($actual -ne $ExpectedExitCode) {
        throw "$Path $($Arguments -join ' ') exited with $actual, expected $ExpectedExitCode"
    }
}

Invoke-ExpectExit -Path $daemon -Arguments @("help") -ExpectedExitCode 0
Invoke-ExpectExit -Path $daemon -Arguments @("--backend", "noop") -ExpectedExitCode 0
Invoke-ExpectExit -Path $daemon -Arguments @("config", "--backend", "noop") -ExpectedExitCode 0
Invoke-ExpectExit -Path $debugClient -Arguments @() -ExpectedExitCode 2

Write-Output "release package smoke passed: $resolvedPackageDir"
