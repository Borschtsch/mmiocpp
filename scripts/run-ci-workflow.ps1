param(
  [Parameter(Mandatory = $true)]
  [string]$Preset,

  [Parameter(Mandatory = $true)]
  [string]$BuildDir
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$resolvedBuildDir = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
  $BuildDir
} else {
  Join-Path $repoRoot $BuildDir
}

New-Item -ItemType Directory -Force -Path $resolvedBuildDir | Out-Null
$workflowLogPath = Join-Path $resolvedBuildDir 'ci-workflow.log'

$cmakePath = Join-Path $repoRoot '.local\cmake\bin\cmake.exe'
if (-not (Test-Path $cmakePath -PathType Leaf)) {
  throw 'Repo-local cmake was not found at .local\\cmake\\bin\\cmake.exe. Run scripts/bootstrap.ps1 -InstallCMake first.'
}

$start = Get-Date
& $cmakePath --workflow --preset $Preset 2>&1 | Tee-Object -FilePath $workflowLogPath
$exitCode = $LASTEXITCODE
$duration = [math]::Round(((Get-Date) - $start).TotalSeconds, 2)

if ($env:GITHUB_ENV) {
  "WORKFLOW_DURATION_SECONDS=$duration" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
  "WORKFLOW_EXIT_CODE=$exitCode" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
  "WORKFLOW_LOG_PATH=$workflowLogPath" | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append
}

if ($exitCode -ne 0) {
  exit $exitCode
}
