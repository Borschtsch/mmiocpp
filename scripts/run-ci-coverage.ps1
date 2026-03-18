param(
  [Parameter(Mandatory = $true)]
  [string]$Preset,

  [Parameter(Mandatory = $true)]
  [string]$BuildDir,

  [string]$OutputDir = '',

  [string]$GcovExe = '',

  [string]$CoverageSource = 'the instrumented test build'
)

$ErrorActionPreference = 'Stop'

& (Join-Path $PSScriptRoot 'run-ci-workflow.ps1') -Preset $Preset -BuildDir $BuildDir

$ciCoverageArguments = @{
  BuildDir = $BuildDir
  CoverageSource = $CoverageSource
}

if (-not [string]::IsNullOrWhiteSpace($OutputDir)) {
  $ciCoverageArguments.OutputDir = $OutputDir
}

if (-not [string]::IsNullOrWhiteSpace($GcovExe)) {
  $ciCoverageArguments.GcovExe = $GcovExe
}

& (Join-Path $PSScriptRoot 'ci-coverage.ps1') @ciCoverageArguments
