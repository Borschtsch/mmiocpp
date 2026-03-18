param(
  [switch]$PortableFallbackOnly,
  [switch]$InstallPortableFallback,
  [switch]$InstallCMake,
  [switch]$InstallArmToolchain,
  [switch]$InstallQemu,
  [switch]$InstallMake
)

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$toolRoot = Join-Path $repoRoot '.local'
$toolchainVersion = '15.2.0posix-13.0.0-ucrt-r4'
$toolchainArchive = 'winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-13.0.0-r4.zip'
$toolchainUrl = "https://github.com/brechtsanders/winlibs_mingw/releases/download/$toolchainVersion/$toolchainArchive"
$toolInstallDir = Join-Path $toolRoot 'winlibs'
$gxx = Join-Path $toolInstallDir 'mingw64\bin\g++.exe'
$cmakeVersion = '4.2.3'
$cmakeArchive = "cmake-$cmakeVersion-windows-x86_64.zip"
$cmakeArchiveUrl = "https://github.com/Kitware/CMake/releases/download/v$cmakeVersion/$cmakeArchive"
$cmakeHashFile = "cmake-$cmakeVersion-SHA-256.txt"
$cmakeHashUrl = "https://github.com/Kitware/CMake/releases/download/v$cmakeVersion/$cmakeHashFile"
$cmakeInstallDir = Join-Path $toolRoot 'cmake'
$repoLocalCmake = Join-Path $cmakeInstallDir 'bin\cmake.exe'
$armToolchainVersion = '15.2.rel1'
$armToolchainArchive = "arm-gnu-toolchain-$armToolchainVersion-mingw-w64-i686-arm-none-eabi.zip"
$armToolchainArchiveUrl = "https://developer.arm.com/-/media/Files/downloads/gnu/$armToolchainVersion/binrel/$armToolchainArchive"
$armToolchainHashFile = "$armToolchainArchive.sha256asc"
$armToolchainHashUrl = "$armToolchainArchiveUrl.sha256asc"
$armToolchainInstallDir = Join-Path $toolRoot 'arm-gnu-toolchain'
$repoLocalArmGcc = Join-Path $armToolchainInstallDir 'bin\arm-none-eabi-gcc.exe'
$repoLocalArmGxx = Join-Path $armToolchainInstallDir 'bin\arm-none-eabi-g++.exe'
$makeInstallDir = Join-Path $toolRoot 'make'
$repoLocalMake = Join-Path $makeInstallDir 'bin\mingw32-make.exe'
$repoLocalMakeIntl = Join-Path $makeInstallDir 'bin\libintl-8.dll'
$repoLocalMakeIconv = Join-Path $makeInstallDir 'bin\libiconv-2.dll'
$qemuVersion = '9.2.4-1'
$qemuArchive = "xpack-qemu-arm-$qemuVersion-win32-x64.zip"
$qemuArchiveUrl = "https://github.com/xpack-dev-tools/qemu-arm-xpack/releases/download/v$qemuVersion/$qemuArchive"
$qemuHashFile = "$qemuArchive.sha"
$qemuHashUrl = "https://github.com/xpack-dev-tools/qemu-arm-xpack/releases/download/v$qemuVersion/$qemuHashFile"
$qemuInstallDir = Join-Path $toolRoot 'qemu'
$repoLocalQemu = Join-Path $qemuInstallDir 'bin\qemu-system-arm.exe'
function Test-ReadableFile {
  param([string]$Path)

  if (-not (Test-Path $Path -PathType Leaf)) {
    return $false
  }

  try {
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $stream.Dispose()
    return $true
  }
  catch {
    return $false
  }
}

function Remove-PathIfExists {
  param([string]$Path)

  if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path)) {
    return
  }

  if (Test-Path $Path -PathType Container) {
    Remove-Item -Path $Path -Recurse -Force
    return
  }

  Remove-Item -Path $Path -Force
}

function Copy-ToolContents {
  param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,
    [Parameter(Mandatory = $true)]
    [string]$DestinationDir
  )

  if (-not (Test-Path $SourceDir -PathType Container)) {
    throw "Expected tool directory was not found: $SourceDir"
  }

  Remove-PathIfExists $DestinationDir
  New-Item -ItemType Directory -Force -Path $DestinationDir | Out-Null
  foreach ($entry in Get-ChildItem -Path $SourceDir -Force) {
    Copy-Item -Path $entry.FullName -Destination $DestinationDir -Recurse -Force
  }
}

function New-TemporaryToolDir {
  $tempRoot = Join-Path $toolRoot '_bootstrap'
  New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
  $tempDir = Join-Path $tempRoot ([guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Force -Path $tempDir | Out-Null
  return $tempDir
}

function Invoke-QuietDownload {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Url,
    [Parameter(Mandatory = $true)]
    [string]$OutFile,
    [Parameter(Mandatory = $true)]
    [string]$Name
  )

  $previousProgressPreference = $ProgressPreference
  try {
    $ProgressPreference = 'SilentlyContinue'
    Write-Host "Downloading $Name from $Url"
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $OutFile
  }
  finally {
    $ProgressPreference = $previousProgressPreference
  }
}

function Get-ExpectedHashFromFile {
  param(
    [Parameter(Mandatory = $true)]
    [string]$HashFilePath,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactName
  )

  foreach ($line in [System.IO.File]::ReadLines($HashFilePath)) {
    if ($line -like "*$ArtifactName*") {
      $match = [regex]::Match($line, '(?i)\b([0-9a-f]{64}|[0-9a-f]{128})\b')
      if ($match.Success) {
        return $match.Groups[1].Value.ToUpperInvariant()
      }
    }
  }

  $fallbackMatch = [regex]::Match([System.IO.File]::ReadAllText($HashFilePath), '(?i)\b([0-9a-f]{64}|[0-9a-f]{128})\b')
  if ($fallbackMatch.Success) {
    return $fallbackMatch.Groups[1].Value.ToUpperInvariant()
  }

  throw "Could not parse the expected hash for $ArtifactName from $HashFilePath"
}

function Get-ArchiveSourceDir {
  param([string]$ExtractDir)

  $entries = @(Get-ChildItem -Path $ExtractDir -Force)
  if ($entries.Count -eq 1 -and $entries[0].PSIsContainer) {
    return $entries[0].FullName
  }

  return $ExtractDir
}

function Install-RepoLocalZipTool {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Name,
    [Parameter(Mandatory = $true)]
    [string]$ArchiveUrl,
    [Parameter(Mandatory = $true)]
    [string]$ArchiveName,
    [Parameter(Mandatory = $true)]
    [string]$DestinationDir,
    [Parameter(Mandatory = $true)]
    [string]$ValidationPath,
    [string]$HashUrl = '',
    [string]$HashFileName = '',
    [ValidateSet('SHA256', 'SHA512')]
    [string]$HashAlgorithm = 'SHA256'
  )

  if (Test-ReadableFile $ValidationPath) {
    Write-Host "Repo-local $Name already installed at $DestinationDir"
    return
  }

  New-Item -ItemType Directory -Force -Path $toolRoot | Out-Null
  $tempDir = New-TemporaryToolDir
  $archivePath = Join-Path $tempDir $ArchiveName
  $hashPath = if ([string]::IsNullOrWhiteSpace($HashFileName)) { '' } else { Join-Path $tempDir $HashFileName }
  $extractDir = Join-Path $tempDir 'extract'

  try {
    Invoke-QuietDownload -Url $ArchiveUrl -OutFile $archivePath -Name "$Name archive"

    if (-not [string]::IsNullOrWhiteSpace($HashUrl)) {
      Invoke-QuietDownload -Url $HashUrl -OutFile $hashPath -Name "$Name checksum"
      $expectedHash = Get-ExpectedHashFromFile -HashFilePath $hashPath -ArtifactName $ArchiveName
      $actualHash = (Get-FileHash -Algorithm $HashAlgorithm -Path $archivePath).Hash.ToUpperInvariant()
      if ($actualHash -ne $expectedHash) {
        throw "$Name archive checksum mismatch. Expected $expectedHash but got $actualHash."
      }
    }

    Write-Host "Extracting $Name into $DestinationDir"
    Expand-Archive -Path $archivePath -DestinationPath $extractDir -Force
    Copy-ToolContents -SourceDir (Get-ArchiveSourceDir -ExtractDir $extractDir) -DestinationDir $DestinationDir
  }
  finally {
    Remove-PathIfExists $tempDir
  }

  if (-not (Test-ReadableFile $ValidationPath)) {
    throw "Repo-local $Name install completed without $ValidationPath"
  }
}

function Test-CMakeAvailable {
  return (Test-ReadableFile $repoLocalCmake)
}

function Test-ArmToolchainAvailable {
  return (Test-ReadableFile $repoLocalArmGcc) -and (Test-ReadableFile $repoLocalArmGxx)
}

function Test-MakeAvailable {
  return (Test-ReadableFile $repoLocalMake) -and
         (Test-ReadableFile $repoLocalMakeIntl) -and
         (Test-ReadableFile $repoLocalMakeIconv)
}

function Test-QemuAvailable {
  return (Test-ReadableFile $repoLocalQemu)
}

function Install-PortableFallback {
  if (Test-Path $gxx) {
    Write-Host "Portable WinLibs toolchain already installed at $toolInstallDir"
    return
  }

  New-Item -ItemType Directory -Force -Path $toolRoot | Out-Null
  $tempDir = New-TemporaryToolDir
  $archivePath = Join-Path $tempDir $toolchainArchive

  try {
    Remove-PathIfExists $toolInstallDir
    Invoke-QuietDownload -Url $toolchainUrl -OutFile $archivePath -Name 'WinLibs toolchain'
    Write-Host "Extracting toolchain into $toolInstallDir"
    Expand-Archive -Path $archivePath -DestinationPath $toolInstallDir -Force
  }
  finally {
    Remove-PathIfExists $tempDir
  }

  if (-not (Test-Path $gxx)) {
    throw "Portable fallback install completed without $gxx"
  }
}

function Install-RepoLocalCMake {
  Install-RepoLocalZipTool `
    -Name 'CMake' `
    -ArchiveUrl $cmakeArchiveUrl `
    -ArchiveName $cmakeArchive `
    -DestinationDir $cmakeInstallDir `
    -ValidationPath $repoLocalCmake `
    -HashUrl $cmakeHashUrl `
    -HashFileName $cmakeHashFile `
    -HashAlgorithm 'SHA256'
}

function Install-RepoLocalArmToolchain {
  Install-RepoLocalZipTool `
    -Name 'Arm GNU Toolchain' `
    -ArchiveUrl $armToolchainArchiveUrl `
    -ArchiveName $armToolchainArchive `
    -DestinationDir $armToolchainInstallDir `
    -ValidationPath $repoLocalArmGcc `
    -HashUrl $armToolchainHashUrl `
    -HashFileName $armToolchainHashFile `
    -HashAlgorithm 'SHA256'

  if (-not (Test-ArmToolchainAvailable)) {
    throw "Repo-local Arm GNU Toolchain install completed without $repoLocalArmGcc and $repoLocalArmGxx"
  }
}

function Copy-MakeRuntimeFiles {
  param([Parameter(Mandatory = $true)][string]$SourceBinDir)

  $destinationBinDir = Join-Path $makeInstallDir 'bin'
  Remove-PathIfExists $makeInstallDir
  New-Item -ItemType Directory -Force -Path $destinationBinDir | Out-Null

  foreach ($leaf in @('mingw32-make.exe', 'libintl-8.dll', 'libiconv-2.dll')) {
    $sourcePath = Join-Path $SourceBinDir $leaf
    if (-not (Test-ReadableFile $sourcePath)) {
      throw "Required GNU make runtime file was not found: $sourcePath"
    }

    Copy-Item -Path $sourcePath -Destination (Join-Path $destinationBinDir $leaf) -Force
  }
}

function Install-RepoLocalMake {
  if (Test-MakeAvailable) {
    Write-Host "Repo-local GNU make already installed at $makeInstallDir"
    return
  }

  $existingWinLibsBinDir = Join-Path $toolInstallDir 'mingw64\bin'
  $existingWinLibsMake = Join-Path $existingWinLibsBinDir 'mingw32-make.exe'
  $existingWinLibsIntl = Join-Path $existingWinLibsBinDir 'libintl-8.dll'
  $existingWinLibsIconv = Join-Path $existingWinLibsBinDir 'libiconv-2.dll'
  if ((Test-ReadableFile $existingWinLibsMake) -and
      (Test-ReadableFile $existingWinLibsIntl) -and
      (Test-ReadableFile $existingWinLibsIconv)) {
    Write-Host "Copying GNU make from existing repo-local WinLibs into $makeInstallDir"
    Copy-MakeRuntimeFiles -SourceBinDir $existingWinLibsBinDir
    if (-not (Test-MakeAvailable)) {
      throw "Repo-local GNU make copy completed without $repoLocalMake, $repoLocalMakeIntl, and $repoLocalMakeIconv"
    }
    return
  }

  New-Item -ItemType Directory -Force -Path $toolRoot | Out-Null
  $tempDir = New-TemporaryToolDir
  $archivePath = Join-Path $tempDir $toolchainArchive
  $extractBinDir = Join-Path $tempDir 'make-bin'

  try {
    Invoke-QuietDownload -Url $toolchainUrl -OutFile $archivePath -Name 'GNU make runtime archive'
    New-Item -ItemType Directory -Force -Path $extractBinDir | Out-Null

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead($archivePath)
    try {
      foreach ($entryName in @('mingw64/bin/mingw32-make.exe', 'mingw64/bin/libintl-8.dll', 'mingw64/bin/libiconv-2.dll')) {
        $entry = $zip.GetEntry($entryName)
        if ($null -eq $entry) {
          throw "Archive entry was not found in ${toolchainArchive}: $entryName"
        }

        $destination = Join-Path $extractBinDir ([System.IO.Path]::GetFileName($entry.FullName))
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $destination, $true)
      }
    }
    finally {
      $zip.Dispose()
    }

    Copy-MakeRuntimeFiles -SourceBinDir $extractBinDir
  }
  finally {
    Remove-PathIfExists $tempDir
  }

  if (-not (Test-MakeAvailable)) {
    throw "Repo-local GNU make install completed without $repoLocalMake, $repoLocalMakeIntl, and $repoLocalMakeIconv"
  }
}

function Install-RepoLocalQemu {
  Install-RepoLocalZipTool `
    -Name 'xPack QEMU Arm' `
    -ArchiveUrl $qemuArchiveUrl `
    -ArchiveName $qemuArchive `
    -DestinationDir $qemuInstallDir `
    -ValidationPath $repoLocalQemu `
    -HashUrl $qemuHashUrl `
    -HashFileName $qemuHashFile `
    -HashAlgorithm 'SHA256'
}
if ($PortableFallbackOnly) {
  $InstallPortableFallback = $true
  $InstallCMake = $false
  $InstallArmToolchain = $false
  $InstallQemu = $false
  $InstallMake = $false
}
elseif (-not ($InstallPortableFallback -or $InstallCMake -or $InstallArmToolchain -or $InstallQemu -or $InstallMake)) {
  $InstallPortableFallback = $true
  $InstallCMake = $true
  $InstallArmToolchain = $true
  $InstallQemu = $true
  $InstallMake = $true
}

if ($InstallPortableFallback) {
  Install-PortableFallback
}

if ($InstallCMake) {
  Install-RepoLocalCMake
}

if ($InstallArmToolchain) {
  Install-RepoLocalArmToolchain
}

if ($InstallMake) {
  Install-RepoLocalMake
}

if ($InstallQemu) {
  Install-RepoLocalQemu
}

Write-Host 'Bootstrap completed.'
Write-Host 'Host build path: .\.local\cmake\bin\cmake.exe --workflow --preset host'
Write-Host 'Host test path: .\.local\cmake\bin\cmake.exe --workflow --preset host-test'
Write-Host 'QEMU build path: .\.local\cmake\bin\cmake.exe --workflow --preset qemu-m3-build'
Write-Host 'QEMU run path: .\.local\cmake\bin\cmake.exe --workflow --preset qemu-m3-run'
Write-Host 'QEMU test path: .\.local\cmake\bin\cmake.exe --workflow --preset qemu-m3-test'
Write-Host 'QEMU Cortex-R5 build path: .\.local\cmake\bin\cmake.exe --workflow --preset qemu-r5-build'
Write-Host 'QEMU Cortex-R5 run path: .\.local\cmake\bin\cmake.exe --workflow --preset qemu-r5-run'
Write-Host 'QEMU Cortex-R5 test path: .\.local\cmake\bin\cmake.exe --workflow --preset qemu-r5-test'
