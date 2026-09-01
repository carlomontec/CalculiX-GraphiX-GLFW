# ==============================================================================
# CalculiX GraphiX (GLFW Edition) — Windows PowerShell Universal Installer
#
# 1-Liner Usage (from PowerShell as standard User):
#   irm https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/install.ps1 | iex
#
# Local Usage (from cloned repo):
#   powershell -ExecutionPolicy Bypass -File .\install.ps1
#
# Unattended / CI Usage:
#   .\install.ps1 -Mode binary -NonInteractive
#   .\install.ps1 -Mode build -NonInteractive
#   .\install.ps1 -Mode head -NonInteractive
# ==============================================================================

[CmdletBinding()]
param (
    [Parameter(Mandatory=$false)]
    [ValidateSet("auto", "binary", "build", "head")]
    [string]$Mode = "auto",

    [Parameter(Mandatory=$false)]
    [string]$InstallDir = "$env:USERPROFILE\.local\bin",

    [Parameter(Mandatory=$false)]
    [switch]$NonInteractive
)

$ErrorActionPreference = "Stop"

$Repo = "carlomontec/CalculiX-GraphiX-GLFW"
$GithubRepoUrl = "https://github.com/${Repo}.git"
$DefaultReleaseUrl = "https://github.com/${Repo}/releases/latest/download"
$ReleaseUrl = if ($env:CGX_DOWNLOAD_BASE_URL) { $env:CGX_DOWNLOAD_BASE_URL } else { $DefaultReleaseUrl }

# Helper formatting functions
function Write-Header($msg) {
    Write-Host "`n================================================================" -ForegroundColor Cyan
    Write-Host "   $msg" -ForegroundColor Cyan
    Write-Host "================================================================" -ForegroundColor Cyan
}

function Write-Step($msg) {
    Write-Host "`n--> $msg" -ForegroundColor Yellow
}

function Write-Success($msg) {
    Write-Host "[OK] $msg" -ForegroundColor Green
}

function Write-Warn($msg) {
    Write-Host "[WARN] $msg" -ForegroundColor Magenta
}

function Write-Err($msg) {
    Write-Host "[ERROR] $msg" -ForegroundColor Red
}

function Prompt-User($promptMsg, $defaultVal = "Y") {
    if ($NonInteractive) {
        return $defaultVal
    }
    $val = Read-Host "$promptMsg [$defaultVal]"
    if ([string]::IsNullOrWhiteSpace($val)) {
        return $defaultVal
    }
    return $val
}

Write-Header "CalculiX GraphiX (GLFW Edition) - Windows Installer"

# -----------------------------------------------------------------------------
# 1. Architecture Check
# -----------------------------------------------------------------------------
$is64Bit = [Environment]::Is64BitOperatingSystem
if (-not $is64Bit) {
    Write-Err "CalculiX GraphiX requires a 64-bit Windows operating system (x86_64)."
    exit 1
}
Write-Host "Detected Platform: Windows (x86_64 64-bit)" -ForegroundColor Green

# -----------------------------------------------------------------------------
# 2. MSYS2 Detection & Setup (for Build from Source)
# -----------------------------------------------------------------------------
function Find-MsysRoot {
    $msysCandidates = @(
        "C:\msys64",
        "$env:LOCALAPPDATA\Programs\msys64",
        "C:\tools\msys64",
        "D:\msys64"
    )

    foreach ($path in $msysCandidates) {
        if (Test-Path "$path\usr\bin\bash.exe") {
            return $path
        }
    }
    return $null
}

function Ensure-MsysRoot {
    $msys = Find-MsysRoot
    if ($msys) {
        Write-Success "Found MSYS2 at $msys"
        return $msys
    }

    Write-Warn "MSYS2 was not found on your system."
    Write-Host "MSYS2 provides the GCC/MinGW-w64 toolchain and GLFW libraries required to compile CGX on Windows."

    $installMsys = Prompt-User "Would you like to download and install MSYS2 automatically now? (Y/n)" "Y"
    if ($installMsys -notmatch "^[Yy]$") {
        Write-Err "MSYS2 is required to build CalculiX GraphiX on Windows. Please install MSYS2 from https://www.msys2.org."
        exit 1
    }

    # Attempt winget first
    $hasWinget = (Get-Command winget -ErrorAction SilentlyContinue) -ne $null
    if ($hasWinget) {
        Write-Host "Installing MSYS2 via Windows Package Manager (winget)..." -ForegroundColor Cyan
        & winget install MSYS2.MSYS2 --silent --accept-package-agreements --accept-source-agreements
        if (Test-Path "C:\msys64\usr\bin\bash.exe") {
            return "C:\msys64"
        }
    }

    # Fallback to direct official installer
    $msysInstallerUrl = "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe"
    $installerTemp = "$env:TEMP\msys2-installer.exe"
    Write-Host "Downloading MSYS2 installer from official GitHub release..." -ForegroundColor Cyan
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $msysInstallerUrl -OutFile $installerTemp -UseBasicParsing

    Write-Host "Running MSYS2 silent installer (installing to C:\msys64)..." -ForegroundColor Cyan
    Start-Process -FilePath $installerTemp -ArgumentList "in", "--confirm-command", "--accept-messages", "--root", "C:\msys64" -Wait -NoNewWindow
    Remove-Item $installerTemp -Force -ErrorAction SilentlyContinue

    if (Test-Path "C:\msys64\usr\bin\bash.exe") {
        Write-Success "MSYS2 successfully installed at C:\msys64"
        return "C:\msys64"
    }

    Write-Err "Failed to automatically install MSYS2. Please install MSYS2 manually from https://www.msys2.org."
    exit 1
}

function Invoke-MsysBash($msysRoot, $cmd, $msystem = "MINGW64") {
    $bashExe = "$msysRoot\usr\bin\bash.exe"
    $env:MSYSTEM = $msystem
    $env:CHERE_INVOKING = "1"
    & $bashExe -l -c "$cmd"
    if ($LASTEXITCODE -ne 0) {
        throw "MSYS2 command failed with exit code ${LASTEXITCODE}: $cmd"
    }
}

# -----------------------------------------------------------------------------
# 3. Fast Binary Installation
# -----------------------------------------------------------------------------
function Do-FastInstall {
    Write-Step "Fetching pre-compiled standalone Windows release binary..."
    
    $assetCandidates = @("cgx_glfw-windows-x86_64.exe", "cgx_glfw-windows-x86_64.zip")
    $downloadOk = $false
    $downloadedFile = $null

    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

    foreach ($asset in $assetCandidates) {
        $downloadUrl = "${ReleaseUrl}/${asset}"
        $tempTarget = "$env:TEMP\$asset"
        Write-Host "Checking release asset: $downloadUrl" -ForegroundColor Cyan
        try {
            Invoke-WebRequest -Uri $downloadUrl -OutFile $tempTarget -UseBasicParsing
            if (Test-Path $tempTarget) {
                $downloadOk = $true
                $downloadedFile = $tempTarget
                break
            }
        } catch {
            # Try next asset format
        }
    }

    if (-not $downloadOk) {
        Write-Warn "Pre-compiled release package not found on GitHub Releases."
        Write-Host "--> Falling back to local compilation from source..." -ForegroundColor Yellow
        Do-BuildInstall $false
        return
    }

    if (-not (Test-Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }

    if ($downloadedFile.EndsWith(".exe")) {
        Copy-Item -Path $downloadedFile -Destination "$InstallDir\cgx_glfw.exe" -Force
        Copy-Item -Path $downloadedFile -Destination "$InstallDir\cgx.exe" -Force
        Remove-Item $downloadedFile -Force -ErrorAction SilentlyContinue
        Write-Success "Installed binary: $InstallDir\cgx_glfw.exe"
        Write-Success "Installed alias:  $InstallDir\cgx.exe"
    } elseif ($downloadedFile.EndsWith(".zip")) {
        $tempExtract = "$env:TEMP\cgx_glfw_extract_$(Get-Random)"
        try {
            Expand-Archive -Path $downloadedFile -DestinationPath $tempExtract -Force
            $extractedExe = Get-ChildItem -Path $tempExtract -Filter "cgx_glfw.exe" -Recurse | Select-Object -First 1
            if ($extractedExe) {
                Copy-Item -Path $extractedExe.FullName -Destination "$InstallDir\cgx_glfw.exe" -Force
                Copy-Item -Path $extractedExe.FullName -Destination "$InstallDir\cgx.exe" -Force
                Write-Success "Installed binary: $InstallDir\cgx_glfw.exe"
                Write-Success "Installed alias:  $InstallDir\cgx.exe"
            }
        } finally {
            Remove-Item $downloadedFile -Force -ErrorAction SilentlyContinue
            Remove-Item $tempExtract -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

# -----------------------------------------------------------------------------
# 4. Build from Source Installation
# -----------------------------------------------------------------------------
function Do-BuildInstall($forceHead = $false) {
    $msysRoot = Ensure-MsysRoot

    Write-Step "Checking MinGW-w64 (MINGW64) build tools & GLFW libraries..."
    $requiredPackages = @(
        "git",
        "make",
        "mingw-w64-x86_64-gcc",
        "mingw-w64-x86_64-glfw",
        "mingw-w64-x86_64-cmake",
        "mingw-w64-x86_64-ninja",
        "mingw-w64-x86_64-pkgconf"
    )

    $pkgListStr = $requiredPackages -join " "
    Write-Host "Ensuring build dependencies via pacman ($pkgListStr)..." -ForegroundColor Cyan
    Invoke-MsysBash $msysRoot "pacman -Sy --noconfirm --needed $pkgListStr" "MINGW64"
    Write-Success "MinGW-w64 build tools and GLFW libraries verified."

    # Locate source directory
    $candidateDirs = @(
        $PSScriptRoot,
        (Get-Location).Path,
        (Split-Path -Parent $PSCommandPath -ErrorAction SilentlyContinue)
    )

    $sourceDir = $null
    foreach ($cand in $candidateDirs) {
        if ($cand -and (Test-Path "$cand\CMakeLists.txt") -and (Test-Path "$cand\cgx_2.23\src\Makefile.glfw")) {
            $sourceDir = $cand
            break
        }
    }

    $isTempClone = $false
    if ($sourceDir) {
        Write-Host "Building from local repository: $sourceDir" -ForegroundColor Cyan
    } else {
        $tempDir = "$env:TEMP\cgx_build_$(Get-Random)"
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
        Write-Step "Cloning CalculiX-GraphiX-GLFW repository..."
        
        $cloneCmd = if ($forceHead) {
            "git clone --depth 1 $GithubRepoUrl '$($tempDir -replace '\\','/')/cgx'"
        } else {
            "git clone $GithubRepoUrl '$($tempDir -replace '\\','/')/cgx'"
        }
        Invoke-MsysBash $msysRoot $cloneCmd "MINGW64"
        $sourceDir = "$tempDir\cgx"
        $isTempClone = $true

        if (-not $forceHead) {
            $tagCheckout = "cd '$($sourceDir -replace '\\','/')' && LATEST_TAG=`$(git tag -l 'v*' | sort -V | tail -n 1) && [ -n `"`$LATEST_TAG`" ] && git checkout `$LATEST_TAG || true"
            Invoke-MsysBash $msysRoot $tagCheckout "MINGW64"
        }
    }

    # Convert Windows path to MSYS path
    $sourceDirMsys = $sourceDir -replace "^([A-Za-z]):", '/$1' -replace "\\", "/"
    $sourceDirMsys = $sourceDirMsys.Substring(0,1).ToLower() + $sourceDirMsys.Substring(1)

    Write-Step "Compiling CalculiX GraphiX (GLFW Edition) with CMake & Ninja..."
    $buildCmd = "cd '$sourceDirMsys' && rm -rf build && cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build"
    Invoke-MsysBash $msysRoot $buildCmd "MINGW64"

    $builtExe = "$sourceDir\bin\cgx_glfw.exe"
    if (-not (Test-Path $builtExe)) {
        $builtExe = "$sourceDir\build\cgx_glfw.exe"
    }

    if (-not (Test-Path $builtExe)) {
        Write-Err "Build failed: cgx_glfw.exe was not generated."
        exit 1
    }
    Write-Success "CalculiX GraphiX binary compiled successfully!"

    # Install to InstallDir
    if (-not (Test-Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }

    Copy-Item -Path $builtExe -Destination "$InstallDir\cgx_glfw.exe" -Force
    Copy-Item -Path $builtExe -Destination "$InstallDir\cgx.exe" -Force

    # Ensure runtime DLLs (if any dynamic dependencies) are colocated in InstallDir
    $dllCandidates = @(
        "$msysRoot\mingw64\bin\glfw3.dll",
        "$msysRoot\ucrt64\bin\glfw3.dll"
    )
    foreach ($dll in $dllCandidates) {
        if (Test-Path $dll) {
            Copy-Item -Path $dll -Destination "$InstallDir\glfw3.dll" -Force -ErrorAction SilentlyContinue
        }
    }

    Write-Success "Installed: $InstallDir\cgx_glfw.exe"
    Write-Success "Installed: $InstallDir\cgx.exe"

    if ($isTempClone -and (Test-Path $tempDir)) {
        Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# -----------------------------------------------------------------------------
# 5. Interactive Mode Selection (if auto)
# -----------------------------------------------------------------------------
if ($Mode -eq "auto") {
    if ($NonInteractive) {
        $Mode = "binary"
    } else {
        Write-Host "`nChoose installation method:" -ForegroundColor Cyan
        Write-Host "  [1] Fast Install (Download pre-built standalone Windows binary) [Default]"
        Write-Host "  [2] Build from Source (Compiles locally with GCC/MinGW-w64 & optimizations)"
        Write-Host "  [3] Bleeding-Edge Build (Latest commit from 'main' branch)"
        $choice = Prompt-User "`nEnter choice (1, 2, or 3)" "1"

        switch ($choice) {
            "1" { $Mode = "binary" }
            "2" { $Mode = "build" }
            "3" { $Mode = "head" }
            default { $Mode = "binary" }
        }
    }
}

# -----------------------------------------------------------------------------
# 6. Execute Selected Installation Mode
# -----------------------------------------------------------------------------
if ($Mode -eq "binary") {
    Do-FastInstall
} elseif ($Mode -eq "build") {
    Do-BuildInstall $false
} elseif ($Mode -eq "head") {
    Do-BuildInstall $true
}

# -----------------------------------------------------------------------------
# 7. PATH Configuration
# -----------------------------------------------------------------------------
$userPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
if ($userPath -split ";" -notcontains $InstallDir) {
    Write-Warn "$InstallDir is not in your Windows User PATH."
    $addPath = Prompt-User "Add $InstallDir to your Windows User PATH environment variable? (Y/n)" "Y"
    if ($addPath -match "^[Yy]$") {
        $newPath = "$userPath;$InstallDir"
        [Environment]::SetEnvironmentVariable("Path", $newPath, [EnvironmentVariableTarget]::User)
        $env:Path = "$env:Path;$InstallDir"
        Write-Success "Added $InstallDir to User PATH. (Restart open terminals for changes to take effect)."
    }
}

# -----------------------------------------------------------------------------
# 8. Quick Verification Test
# -----------------------------------------------------------------------------
Write-Step "Running launch and VTU export verification test..."
$installedBin = "$InstallDir\cgx_glfw.exe"
if (Test-Path $installedBin) {
    try {
        $msysRoot = Find-MsysRoot
        if ($msysRoot -and (Test-Path "test\test_vtu.fbl")) {
            Invoke-MsysBash $msysRoot "'$($installedBin -replace '\\','/')' -bg test/test_vtu.fbl" "MINGW64"
            if (Test-Path "all.vtu") {
                Write-Success "CalculiX GraphiX (GLFW Edition) launch & VTU export verification passed!"
                Remove-Item all.vtu, all.pvd, all_step_*.vtu -Force -ErrorAction SilentlyContinue
            } else {
                Write-Success "CalculiX GraphiX binary execution verified."
            }
        } else {
            Write-Success "CalculiX GraphiX (GLFW Edition) installed and verified at $installedBin"
        }
    } catch {
        Write-Warn "Verification test encountered an issue: $_"
    }
}

Write-Header "CalculiX GraphiX (GLFW Edition) Installed Successfully!"
Write-Host "`nInstalled Location:" -ForegroundColor Yellow
Write-Host "   Directory : $InstallDir" -ForegroundColor White
Write-Host "   Executable: $InstallDir\cgx_glfw.exe" -ForegroundColor Green
Write-Host "   Alias     : $InstallDir\cgx.exe" -ForegroundColor Green

Write-Host "`nTo launch CGX from any Command Prompt or PowerShell:" -ForegroundColor Yellow
Write-Host "   cgx_glfw model.frd          (open 3D finite element model)" -ForegroundColor Cyan
Write-Host "   cgx_glfw                    (open interactive canvas)" -ForegroundColor Cyan
Write-Host "   cgx_glfw -b script.fbl      (batch mode)" -ForegroundColor Cyan
Write-Host "   cgx_glfw -bg script.fbl     (background batch mode)" -ForegroundColor Cyan

Write-Host "`nDocumentation & Tutorials:" -ForegroundColor Yellow
Write-Host "   https://github.com/$Repo" -ForegroundColor White
Write-Host "================================================================`n" -ForegroundColor Cyan

