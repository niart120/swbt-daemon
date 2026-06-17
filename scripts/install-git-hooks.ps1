$ErrorActionPreference = "Stop"

$repoRoot = git rev-parse --show-toplevel
Set-Location $repoRoot

git config core.hooksPath .githooks

$hooksPath = git config --get core.hooksPath
Write-Host "Configured core.hooksPath=$hooksPath"
