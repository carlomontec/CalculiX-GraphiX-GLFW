# ==============================================================================
# CalculiX GraphiX (GLFW Edition) - Windows PowerShell Universal Installer
#
# 1-Liner Usage (from Windows PowerShell 5.1 or PowerShell 7):
#   irm https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/install.ps1 | iex
#   irm https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/uninstall.ps1 | iex
#
# 1-Liner Environment Variables:
#   $env:CGX_MODE            = "auto"   # auto | binary | build | head | uninstall
#   $env:CGX_INSTALL_DIR     = "$env:USERPROFILE\.local\bin"
#   $env:CGX_TAG             = ""       # Specific release tag (e.g. v2.23-glfw-0.11.0)
#   $env:CGX_FORCE           = "1"      # Replace or uninstall without prompting
#   $env:CGX_NON_INTERACTIVE = "1"      # Accept defaults without prompting
#   $env:CGX_KEEP_SOURCE     = "1"      # Keep %USERPROFILE%\.cgx during uninstall
#   $env:CGX_SKIP_PATH       = "1"      # Do not modify User PATH
#
# Local Usage (from cloned repo):
#   powershell -ExecutionPolicy Bypass -File .\install.ps1
#   .\install.ps1 -Mode binary -NonInteractive
#   .\install.ps1 -Mode build -Tag v2.23-glfw-0.11.0
#   .\install.ps1 -Mode head -Force -NonInteractive
#   .\install.ps1 -Mode uninstall
# ==============================================================================

[CmdletBinding()]
param (
    [Parameter(Mandatory=$false)]
    [ValidateSet("auto", "binary", "build", "head", "uninstall")]
    [string]$Mode = $(if ($env:CGX_MODE) { $env:CGX_MODE } else { "auto" }),

    [Parameter(Mandatory=$false)]
    [string]$InstallDir = $(if ($env:CGX_INSTALL_DIR) { $env:CGX_INSTALL_DIR } else { "" }),

    [Parameter(Mandatory=$false)]
    [string]$Tag = $(if ($env:CGX_TAG) { $env:CGX_TAG } else { "" }),

    [Parameter(Mandatory=$false)]
    [switch]$Force,

    [Parameter(Mandatory=$false)]
    [switch]$NonInteractive,

    [Parameter(Mandatory=$false)]
    [switch]$KeepSource,

    [Parameter(Mandatory=$false)]
    [switch]$SkipPath
)

$ErrorActionPreference = "Stop"

if (-not $Force -and $env:CGX_FORCE -match '^(1|true|yes)$') { $Force = $true }
if (-not $NonInteractive -and $env:CGX_NON_INTERACTIVE -match '^(1|true|yes)$') { $NonInteractive = $true }
if (-not $KeepSource -and $env:CGX_KEEP_SOURCE -match '^(1|true|yes)$') { $KeepSource = $true }
if (-not $SkipPath -and $env:CGX_SKIP_PATH -match '^(1|true|yes)$') { $SkipPath = $true }

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
} catch {
    # Ignore on runtimes where TLS 1.2+ is already managed by OS defaults
}

$scriptPath = $MyInvocation.MyCommand.Path
$script:IsScriptFile = -not [string]::IsNullOrWhiteSpace($scriptPath)
$script:ScriptDir = if ($script:IsScriptFile) { Split-Path -Parent $scriptPath } else { $null }
$script:MsysRoot = $null

$Repo = "carlomontec/CalculiX-GraphiX-GLFW"
$GithubRepoUrl = "https://github.com/${Repo}.git"
$DefaultReleaseUrl = "https://github.com/${Repo}/releases/latest/download"

# -----------------------------------------------------------------------------
# Formatting, Prompting & Error Helpers
# -----------------------------------------------------------------------------
function Write-Header([string]$msg) {
    Write-Host "`n================================================================" -ForegroundColor Cyan
    Write-Host "   $msg" -ForegroundColor Cyan
    Write-Host "================================================================" -ForegroundColor Cyan
}

function Write-Step([string]$msg) {
    Write-Host "`n--> $msg" -ForegroundColor Yellow
}

function Write-Success([string]$msg) {
    Write-Host "[OK] $msg" -ForegroundColor Green
}

function Write-Warn([string]$msg) {
    Write-Host "[WARN] $msg" -ForegroundColor Magenta
}

function Write-Err([string]$msg) {
    Write-Host "[ERROR] $msg" -ForegroundColor Red
}

function Fail-Install([string]$msg) {
    Write-Err $msg
    if ($script:IsScriptFile) {
        exit 1
    }
    # Under 'irm | iex', exit would terminate the user's interactive PowerShell host
    throw $msg
}

function Prompt-Value([string]$promptMsg, [string]$defaultVal) {
    if ($NonInteractive) {
        return $defaultVal
    }
    $val = Read-Host "$promptMsg [$defaultVal]"
    if ([string]::IsNullOrWhiteSpace($val)) {
        return $defaultVal
    }
    return $val.Trim()
}

function Confirm-User([string]$question, [string]$defaultVal = "Y") {
    if ($NonInteractive) {
        return ($defaultVal -match '^[Yy]')
    }
    $hint = if ($defaultVal -match '^[Yy]') { "Y/n" } else { "y/N" }
    $val = Read-Host "$question ($hint)"
    if ([string]::IsNullOrWhiteSpace($val)) {
        $val = $defaultVal
    }
    return ($val.Trim() -match '^[Yy]')
}

# -----------------------------------------------------------------------------
# Path, Download, Binary Inspection & MSYS2 Helpers
# -----------------------------------------------------------------------------
function Resolve-NormalizedPath([string]$pathVal) {
    $p = $pathVal.Trim()
    if ($p -eq "~") {
        $p = $env:USERPROFILE
    } elseif ($p -match '^~[\\/]') {
        $p = Join-Path $env:USERPROFILE ($p.Substring(2))
    }
    $p = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($p)
    if ($p.Length -gt 3) {
        $p = $p.TrimEnd('\', '/')
    }
    return $p
}

function ConvertTo-MsysPath([string]$winPath) {
    $p = $winPath.Trim() -replace '\\', '/'
    if ($p -match '^([A-Za-z]):/(.*)$') {
        $rest = $Matches[2].TrimEnd('/')
        if ([string]::IsNullOrEmpty($rest)) {
            return "/" + $Matches[1].ToLower()
        }
        return "/" + $Matches[1].ToLower() + "/" + $rest
    }
    if ($p -match '^([A-Za-z]):$') {
        return "/" + $Matches[1].ToLower()
    }
    return $p
}

function Invoke-DownloadFile([string]$Url, [string]$OutFile) {
    if (Test-Path $OutFile) {
        Remove-Item $OutFile -Force -ErrorAction SilentlyContinue
    }
    $prevProgress = $ProgressPreference
    try {
        # Disable PowerShell 5.1 progress bar rendering to keep downloads fast
        $ProgressPreference = "SilentlyContinue"
        Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing
    } finally {
        $ProgressPreference = $prevProgress
    }
}

function Get-ShortSha256([string]$FilePath) {
    return (Get-FileHash -Path $FilePath -Algorithm SHA256).Hash.Substring(0, 12).ToLower()
}

function Format-ProcessArguments([string[]]$Arguments) {
    if (-not $Arguments -or $Arguments.Count -eq 0) {
        return ""
    }
    $quoted = foreach ($arg in $Arguments) {
        if ($null -eq $arg) {
            '""'
        } elseif ($arg -match '[\s"]') {
            '"' + ($arg -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
        } else {
            $arg
        }
    }
    return ($quoted -join " ")
}

function Invoke-CgxProbe {
    param (
        [Parameter(Mandatory=$true)][string]$ExePath,
        [Parameter(Mandatory=$false)][string[]]$Arguments = @(),
        [Parameter(Mandatory=$false)][int]$TimeoutMs = 5000
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ExePath
    $psi.Arguments = Format-ProcessArguments $Arguments
    $psi.WorkingDirectory = (Get-Location).ProviderPath
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi
    try {
        [void]$proc.Start()
        $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
        $stderrTask = $proc.StandardError.ReadToEndAsync()

        $exited = $proc.WaitForExit($TimeoutMs)
        if (-not $exited) {
            try { $proc.Kill() } catch {}
            return [pscustomobject]@{
                TimedOut = $true
                ExitCode = $null
                StdOut   = ""
                StdErr   = ""
            }
        }

        # Complete async stream flush before reading outputs and ExitCode
        $proc.WaitForExit()
        return [pscustomobject]@{
            TimedOut = $false
            ExitCode = $proc.ExitCode
            StdOut   = $stdoutTask.GetAwaiter().GetResult()
            StdErr   = $stderrTask.GetAwaiter().GetResult()
        }
    } finally {
        $proc.Dispose()
    }
}

function Get-CgxBinaryVersion([string]$ExePath) {
    if (-not (Test-Path $ExePath)) {
        return $null
    }
    try {
        $res = Invoke-CgxProbe -ExePath $ExePath -Arguments @("--version") -TimeoutMs 3000
        if ($res.TimedOut) {
            return $null
        }
        $combined = ($res.StdOut + "`n" + $res.StdErr)
        if ($combined -match 'Version\s+([0-9][^\r\n]*)') {
            return $Matches[1].Trim()
        }
    } catch {
        return $null
    }
    return $null
}

function Find-MsysRoot {
    $candidates = @()
    if ($env:MSYS2_ROOT) { $candidates += $env:MSYS2_ROOT }
    $candidates += @(
        "C:\msys64",
        "$env:LOCALAPPDATA\Programs\msys64",
        "C:\tools\msys64",
        "D:\msys64"
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path (Join-Path $candidate "usr\bin\bash.exe"))) {
            return $candidate.TrimEnd('\', '/')
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

    if (-not (Confirm-User "Would you like to download and install MSYS2 automatically now?" "Y")) {
        Fail-Install "MSYS2 is required to build CalculiX GraphiX on Windows. Please install MSYS2 from https://www.msys2.org."
    }

    $hasWinget = $null -ne (Get-Command winget -ErrorAction SilentlyContinue)
    if ($hasWinget) {
        Write-Host "Installing MSYS2 via Windows Package Manager (winget)..." -ForegroundColor Cyan
        $prevEap = $ErrorActionPreference
        try {
            $ErrorActionPreference = "Continue"
            & winget install MSYS2.MSYS2 --silent --accept-package-agreements --accept-source-agreements
        } finally {
            $ErrorActionPreference = $prevEap
        }
        $msys = Find-MsysRoot
        if ($msys) {
            Write-Success "MSYS2 installed at $msys"
            return $msys
        }
    }

    $msysInstallerUrl = "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe"
    $installerTemp = Join-Path $env:TEMP ("msys2-installer-" + [System.Guid]::NewGuid().ToString("N") + ".exe")
    Write-Host "Downloading MSYS2 installer from official GitHub release..." -ForegroundColor Cyan
    try {
        Invoke-DownloadFile -Url $msysInstallerUrl -OutFile $installerTemp
        Write-Host "Running MSYS2 silent installer (installing to C:\msys64)..." -ForegroundColor Cyan
        Start-Process -FilePath $installerTemp -ArgumentList "in", "--confirm-command", "--accept-messages", "--root", "C:\msys64" -Wait -NoNewWindow
    } finally {
        Remove-Item $installerTemp -Force -ErrorAction SilentlyContinue
    }

    $msys = Find-MsysRoot
    if ($msys) {
        Write-Success "MSYS2 successfully installed at $msys"
        return $msys
    }

    Fail-Install "Failed to automatically install MSYS2. Please install MSYS2 manually from https://www.msys2.org."
}

function Invoke-MsysBashScript {
    param (
        [Parameter(Mandatory=$true)][string]$MsysRoot,
        [Parameter(Mandatory=$true)][string]$ScriptContent,
        [Parameter(Mandatory=$false)][string]$Msystem = "UCRT64",
        [Parameter(Mandatory=$false)][switch]$AllowNonZero,
        [Parameter(Mandatory=$false)][switch]$CaptureOutput
    )

    $bashExe = Join-Path $MsysRoot "usr\bin\bash.exe"
    $tmpScript = Join-Path $env:TEMP ("cgx_msys_" + [System.Guid]::NewGuid().ToString("N") + ".sh")
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    # Write a temp LF script file so PowerShell 5.1 never mangles embedded double quotes
    [System.IO.File]::WriteAllText($tmpScript, ($ScriptContent -replace "`r`n", "`n") + "`n", $utf8NoBom)

    $savedEnv = @{}
    $envToSet = @{
        "MSYSTEM"        = $Msystem
        "CHERE_INVOKING" = "1"
    }
    foreach ($k in $envToSet.Keys) {
        $savedEnv[$k] = [Environment]::GetEnvironmentVariable($k, [EnvironmentVariableTarget]::Process)
        [Environment]::SetEnvironmentVariable($k, $envToSet[$k], [EnvironmentVariableTarget]::Process)
    }

    $prevEap = $ErrorActionPreference
    $captured = $null
    $exitCode = 1
    try {
        $ErrorActionPreference = "Continue"
        $msysScriptArg = ConvertTo-MsysPath $tmpScript
        if ($CaptureOutput) {
            $captured = & $bashExe -l $msysScriptArg 2>&1
            $exitCode = $LASTEXITCODE
        } else {
            & $bashExe -l $msysScriptArg
            $exitCode = $LASTEXITCODE
        }
    } finally {
        $ErrorActionPreference = $prevEap
        foreach ($k in $savedEnv.Keys) {
            [Environment]::SetEnvironmentVariable($k, $savedEnv[$k], [EnvironmentVariableTarget]::Process)
        }
        Remove-Item $tmpScript -Force -ErrorAction SilentlyContinue
    }

    if (-not $AllowNonZero -and $exitCode -ne 0) {
        if ($CaptureOutput -and $captured) {
            $captured | ForEach-Object { Write-Host $_ }
        }
        Fail-Install "MSYS2 command failed with exit code $exitCode."
    }

    if ($CaptureOutput) {
        return $captured
    }
}

# -----------------------------------------------------------------------------
# Existing Installation Detection & Safe Replacement
# -----------------------------------------------------------------------------
function Get-WindowsPathLayers {
    $machineRaw = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::Machine)
    $userRaw = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)

    $machineEntries = @()
    if ($machineRaw) {
        $machineEntries = @($machineRaw -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object { $_.Trim() })
    }
    $userEntries = @()
    if ($userRaw) {
        $userEntries = @($userRaw -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object { $_.Trim() })
    }

    return [pscustomobject]@{
        Machine = $machineEntries
        User    = $userEntries
        All     = @($machineEntries + $userEntries)
    }
}

function Test-PathEntryPresent([string[]]$pathEntries, [string]$targetDir) {
    $normTarget = $targetDir.Trim().TrimEnd('\', '/')
    foreach ($entry in $pathEntries) {
        if ($entry -and ($entry.Trim().TrimEnd('\', '/') -ieq $normTarget)) {
            return $true
        }
    }
    return $false
}

function Get-CgxBinariesInDir([string]$Dir) {
    $found = @()
    if (-not $Dir -or -not (Test-Path $Dir)) {
        return $found
    }
    foreach ($name in @("cgx_glfw.exe", "cgx.exe", "cgx_dev.exe")) {
        $fullPath = Join-Path $Dir $name
        if (Test-Path $fullPath) {
            $found += $fullPath
        }
    }
    return $found
}

function Write-CgxBinaryDetails([string[]]$Paths) {
    foreach ($fullPath in $Paths) {
        if (-not (Test-Path $fullPath)) { continue }
        $item = Get-Item $fullPath -Force
        $ver = Get-CgxBinaryVersion $fullPath
        $verText = if ($ver) { "Version $ver, " } else { "" }
        $modStr = $item.LastWriteTime.ToString("yyyy-MM-dd HH:mm")
        $sizeMb = "{0:N2} MB" -f ($item.Length / 1MB)
        $hash12 = Get-ShortSha256 $fullPath
        Write-Host "  - $fullPath (${verText}modified $modStr, $sizeMb, sha256: $hash12)" -ForegroundColor Yellow
    }
}

function Test-ExistingInstallation([string]$TargetDir, [string]$AliasName) {
    $targetFiles = @("cgx_glfw.exe", $AliasName)
    $willOverwrite = $false
    foreach ($name in $targetFiles) {
        if (Test-Path (Join-Path $TargetDir $name)) {
            $willOverwrite = $true
            break
        }
    }

    if (-not $willOverwrite) {
        return $true
    }

    Write-Step "Existing installation detected in ${TargetDir}:"
    Write-CgxBinaryDetails (Get-CgxBinariesInDir $TargetDir)

    $normTarget = $TargetDir.TrimEnd('\', '/')
    $layers = Get-WindowsPathLayers
    $otherMatches = @()
    foreach ($dir in $layers.All) {
        $normDir = $dir.TrimEnd('\', '/')
        if (-not $normDir -or ($normDir -ieq $normTarget) -or -not (Test-Path $dir)) {
            continue
        }
        foreach ($name in @("cgx_glfw.exe", "cgx.exe", "cgx_dev.exe")) {
            $candidate = Join-Path $dir $name
            if (Test-Path $candidate) {
                $otherMatches += $candidate
            }
        }
    }
    $otherMatches = @($otherMatches | Select-Object -Unique)
    if ($otherMatches.Count -gt 0) {
        Write-Host "  Note: Other CGX binary(s) also found in Windows PATH:" -ForegroundColor Gray
        foreach ($m in $otherMatches) {
            Write-Host "    - $m" -ForegroundColor Gray
        }
    }

    if ($Force -or $NonInteractive) {
        Write-Host "Replacing existing installation in $TargetDir..." -ForegroundColor Cyan
        return $true
    }

    if (-not (Confirm-User "Would you like to replace the existing installation?" "Y")) {
        Write-Host "Installation cancelled. Existing binaries were not modified." -ForegroundColor Yellow
        return $false
    }

    return $true
}

function Stop-LockingProcesses([string]$TargetPath) {
    $procs = @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
        $_.ProcessName -in @("cgx_glfw", "cgx", "cgx_dev")
    })

    $lockingProcs = @()
    foreach ($proc in $procs) {
        try {
            if ($proc.MainModule -and ($proc.MainModule.FileName -ieq $TargetPath)) {
                $lockingProcs += $proc
            }
        } catch {
            # Ignore AccessDenied when inspecting protected processes
        }
    }

    if ($lockingProcs.Count -eq 0) {
        return
    }

    $pidList = ($lockingProcs | ForEach-Object { $_.Id }) -join ", "
    Write-Warn "Running process(es) are locking $TargetPath (PID: $pidList)."
    if ($Force -or $NonInteractive -or (Confirm-User "Stop running process(es) and continue?" "Y")) {
        foreach ($lp in $lockingProcs) {
            Stop-Process -Id $lp.Id -Force -ErrorAction Stop
        }
        Start-Sleep -Milliseconds 300
    } else {
        Fail-Install "Cannot replace $TargetPath while it is running. Close CalculiX GraphiX and re-run the installer."
    }
}

function Install-BinarySafely {
    param (
        [Parameter(Mandatory=$true)][string]$StagedExePath,
        [Parameter(Mandatory=$true)][string]$TargetDir,
        [Parameter(Mandatory=$true)][string]$AliasName
    )

    if (-not (Test-Path $StagedExePath)) {
        Fail-Install "Staged executable not found at $StagedExePath."
    }

    Unblock-File -Path $StagedExePath -ErrorAction SilentlyContinue

    $probe = Invoke-CgxProbe -ExePath $StagedExePath -Arguments @("--version") -TimeoutMs 5000
    $probeOut = ($probe.StdOut + "`n" + $probe.StdErr).Trim()
    if ($probe.TimedOut) {
        Fail-Install "Pre-install sanity check timed out on staged binary: $StagedExePath"
    }
    if ($probe.ExitCode -ne 0 -and ($probeOut -notmatch 'Version\s+[0-9]')) {
        Fail-Install "Pre-install sanity check failed (exit code $($probe.ExitCode)): $probeOut"
    }

    if (-not (Test-Path $TargetDir)) {
        New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
    }

    $targets = @(
        @{ Name = "cgx_glfw.exe"; Label = "binary" },
        @{ Name = $AliasName;     Label = "alias " }
    )

    foreach ($t in $targets) {
        $destPath = Join-Path $TargetDir $t.Name
        Stop-LockingProcesses $destPath

        if (Test-Path $destPath) {
            $destItem = Get-Item $destPath -Force -ErrorAction SilentlyContinue
            if ($destItem -and $destItem.IsReadOnly) {
                $destItem.IsReadOnly = $false
            }
        }

        $tmpDest = Join-Path $TargetDir ("." + $t.Name + ".tmp." + [System.Guid]::NewGuid().ToString("N"))
        try {
            Copy-Item -Path $StagedExePath -Destination $tmpDest -Force
            Unblock-File -Path $tmpDest -ErrorAction SilentlyContinue
            Move-Item -Path $tmpDest -Destination $destPath -Force
        } catch {
            Remove-Item $tmpDest -Force -ErrorAction SilentlyContinue
            throw
        }

        Write-Success "Installed $($t.Label): $destPath"
    }
}

# -----------------------------------------------------------------------------
# Fast Binary Installation
# -----------------------------------------------------------------------------
function Verify-Sha256([string]$FilePath, [string]$AssetName, [string]$BaseReleaseUrl) {
    $sumsUrl = "${BaseReleaseUrl}/SHA256SUMS.txt"
    $sumsFile = Join-Path $env:TEMP ("cgx_SHA256SUMS_" + [System.Guid]::NewGuid().ToString("N") + ".txt")
    try {
        Invoke-DownloadFile -Url $sumsUrl -OutFile $sumsFile
    } catch {
        Remove-Item $sumsFile -Force -ErrorAction SilentlyContinue
        Write-Warn "SHA256SUMS.txt not found at release URL; skipping checksum verification."
        return
    }

    try {
        $expectedHash = $null
        $escapedName = [regex]::Escape($AssetName)
        foreach ($line in (Get-Content -Path $sumsFile)) {
            if ($line -match "^([0-9a-fA-F]{64})\s+\*?(\./)?${escapedName}\s*$") {
                $expectedHash = $matches[1].ToLower()
                break
            }
        }
        if (-not $expectedHash) {
            Write-Warn "'$AssetName' not listed in SHA256SUMS.txt; skipping checksum verification."
            return
        }
        $actualHash = (Get-FileHash -Path $FilePath -Algorithm SHA256).Hash.ToLower()
        if ($actualHash -ne $expectedHash) {
            Remove-Item $FilePath -Force -ErrorAction SilentlyContinue
            Fail-Install "SHA-256 checksum mismatch for $AssetName (expected $expectedHash, got $actualHash)."
        }
        Write-Success "SHA-256 checksum verified for $AssetName."
    } finally {
        Remove-Item $sumsFile -Force -ErrorAction SilentlyContinue
    }
}

function Do-FastInstall([string]$TargetDir, [string]$AliasName, [string]$BaseReleaseUrl, [string]$RequestedTag) {
    Write-Step "Fetching pre-compiled standalone Windows release binary..."

    $assetCandidates = @("cgx_glfw-windows-x86_64.exe", "cgx_glfw-windows-x86_64.zip")
    $downloadOk = $false
    $downloadedFile = $null
    $downloadedAsset = $null
    $tempExtract = $null

    try {
        foreach ($asset in $assetCandidates) {
            $downloadUrl = "${BaseReleaseUrl}/${asset}"
            $tempTarget = Join-Path $env:TEMP ("cgx_dl_" + [System.Guid]::NewGuid().ToString("N") + "_" + $asset)
            Write-Host "Checking release asset: $downloadUrl" -ForegroundColor Cyan
            try {
                Invoke-DownloadFile -Url $downloadUrl -OutFile $tempTarget
                if ((Test-Path $tempTarget) -and ((Get-Item $tempTarget).Length -gt 0)) {
                    $downloadOk = $true
                    $downloadedFile = $tempTarget
                    $downloadedAsset = $asset
                    break
                }
                Remove-Item $tempTarget -Force -ErrorAction SilentlyContinue
            } catch {
                Remove-Item $tempTarget -Force -ErrorAction SilentlyContinue
                Write-Warn "Could not download ${asset}: $($_.Exception.Message)"
            }
        }

        if (-not $downloadOk) {
            if (-not [string]::IsNullOrWhiteSpace($RequestedTag)) {
                Fail-Install "Pre-compiled release asset was not found for tag '$RequestedTag' at $BaseReleaseUrl."
            }
            Write-Warn "Pre-compiled release package not found on GitHub Releases."
            Write-Host "--> Falling back to local compilation from source..." -ForegroundColor Yellow
            Do-BuildInstall -ForceHead $false -TargetDir $TargetDir -AliasName $AliasName -RequestedTag $RequestedTag
            return
        }

        Verify-Sha256 -FilePath $downloadedFile -AssetName $downloadedAsset -BaseReleaseUrl $BaseReleaseUrl

        if ($downloadedFile.EndsWith(".exe")) {
            Install-BinarySafely -StagedExePath $downloadedFile -TargetDir $TargetDir -AliasName $AliasName
        } elseif ($downloadedFile.EndsWith(".zip")) {
            $tempExtract = Join-Path $env:TEMP ("cgx_glfw_extract_" + [System.Guid]::NewGuid().ToString("N"))
            Expand-Archive -Path $downloadedFile -DestinationPath $tempExtract -Force
            $extractedExe = Get-ChildItem -Path $tempExtract -Filter "cgx_glfw.exe" -Recurse | Select-Object -First 1
            if (-not $extractedExe) {
                Fail-Install "Downloaded archive '$downloadedAsset' did not contain cgx_glfw.exe."
            }
            Install-BinarySafely -StagedExePath $extractedExe.FullName -TargetDir $TargetDir -AliasName $AliasName
        }
    } finally {
        if ($downloadedFile) { Remove-Item $downloadedFile -Force -ErrorAction SilentlyContinue }
        if ($tempExtract) { Remove-Item $tempExtract -Recurse -Force -ErrorAction SilentlyContinue }
    }
}

# -----------------------------------------------------------------------------
# Build from Source Installation (MSYS2 UCRT64)
# -----------------------------------------------------------------------------
function Do-BuildInstall {
    param (
        [Parameter(Mandatory=$true)][bool]$ForceHead,
        [Parameter(Mandatory=$true)][string]$TargetDir,
        [Parameter(Mandatory=$true)][string]$AliasName,
        [Parameter(Mandatory=$false)][string]$RequestedTag = ""
    )

    $script:MsysRoot = Ensure-MsysRoot

    Write-Step "Checking MinGW-w64 (UCRT64) build tools & GLFW libraries..."
    $requiredPackages = @(
        "git",
        "make",
        "mingw-w64-ucrt-x86_64-gcc",
        "mingw-w64-ucrt-x86_64-glfw",
        "mingw-w64-ucrt-x86_64-cmake",
        "mingw-w64-ucrt-x86_64-ninja",
        "mingw-w64-ucrt-x86_64-pkgconf"
    )

    $pkgListStr = $requiredPackages -join " "
    Write-Host "Ensuring build dependencies via pacman ($pkgListStr)..." -ForegroundColor Cyan
    Invoke-MsysBashScript -MsysRoot $script:MsysRoot -ScriptContent @"
set -e
pacman -Sy --noconfirm --needed $pkgListStr
"@
    Write-Success "MinGW-w64 (UCRT64) build tools and GLFW libraries verified."

    $candidateDirs = @()
    if (-not [string]::IsNullOrWhiteSpace($script:ScriptDir)) {
        $candidateDirs += $script:ScriptDir
    }
    $cwdPath = (Get-Location).ProviderPath
    if (-not [string]::IsNullOrWhiteSpace($cwdPath)) {
        $candidateDirs += $cwdPath
    }

    $sourceDir = $null
    foreach ($cand in $candidateDirs) {
        if ((Test-Path (Join-Path $cand "CMakeLists.txt")) -and (Test-Path (Join-Path $cand "cgx_2.23\src\Makefile.glfw"))) {
            $sourceDir = $cand
            break
        }
    }

    $tempDir = $null
    $isTempClone = $false
    try {
        if ($sourceDir) {
            Write-Host "Building from local repository: $sourceDir" -ForegroundColor Cyan
        } else {
            $tempDir = Join-Path $env:TEMP ("cgx_build_" + [System.Guid]::NewGuid().ToString("N"))
            New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
            $sourceDir = Join-Path $tempDir "cgx"
            $sourceDirMsys = ConvertTo-MsysPath $sourceDir
            $isTempClone = $true

            Write-Step "Cloning CalculiX-GraphiX-GLFW repository..."
            if ($ForceHead) {
                Invoke-MsysBashScript -MsysRoot $script:MsysRoot -ScriptContent @"
set -e
git clone --depth 1 '$GithubRepoUrl' '$sourceDirMsys'
"@
            } elseif (-not [string]::IsNullOrWhiteSpace($RequestedTag)) {
                Invoke-MsysBashScript -MsysRoot $script:MsysRoot -ScriptContent @"
set -e
git clone '$GithubRepoUrl' '$sourceDirMsys'
cd '$sourceDirMsys'
git checkout -f '$RequestedTag'
"@
            } else {
                Invoke-MsysBashScript -MsysRoot $script:MsysRoot -ScriptContent @"
set -e
git clone '$GithubRepoUrl' '$sourceDirMsys'
cd '$sourceDirMsys'
LATEST_TAG=`$(git tag -l 'v*glfw*' --sort=-v:refname | head -n 1)
if [ -z "`$LATEST_TAG" ]; then
    LATEST_TAG=`$(git tag -l 'v*' --sort=-v:refname | head -n 1)
fi
if [ -n "`$LATEST_TAG" ] && git cat-file -e "`${LATEST_TAG}:CMakeLists.txt" 2>/dev/null; then
    echo "Checking out latest release tag: `$LATEST_TAG"
    git checkout -f "`$LATEST_TAG"
else
    echo "No release tag found; using default main branch."
fi
"@
            }
        }

        $sourceDirMsys = ConvertTo-MsysPath $sourceDir
        Write-Step "Compiling CalculiX GraphiX (GLFW Edition) with CMake & Ninja..."
        Invoke-MsysBashScript -MsysRoot $script:MsysRoot -ScriptContent @"
set -e
cd '$sourceDirMsys'
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
"@

        $builtExe = Join-Path $sourceDir "bin\cgx_glfw.exe"
        if (-not (Test-Path $builtExe)) {
            $builtExe = Join-Path $sourceDir "build\cgx_glfw.exe"
        }
        if (-not (Test-Path $builtExe)) {
            Fail-Install "Build failed: cgx_glfw.exe was not generated."
        }
        Write-Success "CalculiX GraphiX binary compiled successfully!"

        Install-BinarySafely -StagedExePath $builtExe -TargetDir $TargetDir -AliasName $AliasName

        # Only colocate glfw3.dll if the compiled executable dynamically imports it
        $builtExeMsys = ConvertTo-MsysPath $builtExe
        $dllOutput = Invoke-MsysBashScript -MsysRoot $script:MsysRoot -AllowNonZero -CaptureOutput -ScriptContent @"
objdump -p '$builtExeMsys' | grep -i "DLL Name:" || true
"@
        $needsGlfwDll = $false
        foreach ($line in $dllOutput) {
            if ($line -match 'DLL Name:\s*glfw3\.dll') {
                $needsGlfwDll = $true
                break
            }
        }
        if ($needsGlfwDll) {
            $ucrtGlfwDll = Join-Path $script:MsysRoot "ucrt64\bin\glfw3.dll"
            if (Test-Path $ucrtGlfwDll) {
                Copy-Item -Path $ucrtGlfwDll -Destination (Join-Path $TargetDir "glfw3.dll") -Force
                Write-Success "Copied required runtime dependency: $TargetDir\glfw3.dll"
            }
        }
    } finally {
        if ($isTempClone -and $tempDir -and (Test-Path $tempDir)) {
            Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

# -----------------------------------------------------------------------------
# Optional Runtime Tools (Gnuplot & FFmpeg)
# -----------------------------------------------------------------------------
function Install-RuntimeTools {
    Write-Step "Checking optional runtime tools (gnuplot, ffmpeg)..."

    $hasGnuplot = $null -ne (Get-Command gnuplot -ErrorAction SilentlyContinue)
    $msysRoot = Find-MsysRoot
    if (-not $hasGnuplot -and $msysRoot) {
        $msysGnuplot = Join-Path $msysRoot "ucrt64\bin\gnuplot.exe"
        if (Test-Path $msysGnuplot) {
            $hasGnuplot = $true
        }
    }

    if ($hasGnuplot) {
        Write-Success "gnuplot is available for 2D engineering plots."
    } elseif (-not $NonInteractive) {
        Write-Host "Gnuplot was not found. It is used for 2D engineering plots ('graph' command)." -ForegroundColor Gray
        if (Confirm-User "Would you like to install Gnuplot now?" "Y") {
            $installed = $false
            $hasWinget = $null -ne (Get-Command winget -ErrorAction SilentlyContinue)
            if ($hasWinget) {
                Write-Host "Installing Gnuplot via winget..." -ForegroundColor Cyan
                $prevEap = $ErrorActionPreference
                try {
                    $ErrorActionPreference = "Continue"
                    & winget install --id Gnuplot.Gnuplot --exact --silent --accept-package-agreements --accept-source-agreements
                    if ($LASTEXITCODE -eq 0) {
                        $installed = $true
                        Write-Success "Gnuplot installed via winget."
                    }
                } catch {
                    # Fall through cleanly
                } finally {
                    $ErrorActionPreference = $prevEap
                }
            } elseif ($msysRoot) {
                Write-Host "Installing Gnuplot via MSYS2 pacman..." -ForegroundColor Cyan
                try {
                    Invoke-MsysBashScript -MsysRoot $msysRoot -AllowNonZero -ScriptContent @"
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gnuplot
"@
                    if (Test-Path (Join-Path $msysRoot "ucrt64\bin\gnuplot.exe")) {
                        $installed = $true
                        Write-Success "Gnuplot installed in MSYS2 UCRT64."
                    }
                } catch {
                    # Fall through cleanly
                }
            }

            if (-not $installed) {
                Write-Warn "Could not automatically install Gnuplot. Tip: run 'winget install Gnuplot.Gnuplot' later."
            }
        } else {
            Write-Host "Tip: Install gnuplot ('winget install Gnuplot.Gnuplot') for 2D engineering plots." -ForegroundColor Cyan
        }
    } else {
        Write-Host "Tip: Install gnuplot ('winget install Gnuplot.Gnuplot') for 2D engineering plots." -ForegroundColor Cyan
    }

    $hasFfmpeg = $null -ne (Get-Command ffmpeg -ErrorAction SilentlyContinue)
    if ($hasFfmpeg) {
        Write-Success "ffmpeg is available for MP4 video recording."
    } else {
        Write-Host "Tip: Install ffmpeg ('winget install Gyan.FFmpeg') to enable MP4 video recording." -ForegroundColor Cyan
    }
}

# -----------------------------------------------------------------------------
# PATH Configuration & PATH Shadow Detection
# -----------------------------------------------------------------------------
function Ensure-PathConfigured([string]$TargetDir) {
    if ($SkipPath) {
        return
    }

    $userRaw = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
    $userEntries = @()
    if ($userRaw) {
        $userEntries = @($userRaw -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object { $_.Trim() })
    }

    if (-not (Test-PathEntryPresent $userEntries $TargetDir)) {
        Write-Warn "$TargetDir is not in your Windows User PATH."
        if (Confirm-User "Add $TargetDir to your Windows User PATH environment variable?" "Y") {
            $userEntries += $TargetDir
            $newPath = $userEntries -join ';'
            [Environment]::SetEnvironmentVariable("Path", $newPath, [EnvironmentVariableTarget]::User)
            Write-Success "Added $TargetDir to User PATH. (Open a new terminal for PATH changes to take effect)."
        }
    }

    $procEntries = @($env:Path -split ';' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if (-not (Test-PathEntryPresent $procEntries $TargetDir)) {
        $env:Path = "$TargetDir;$env:Path"
    }
}

function Test-PathShadowing([string]$TargetDir, [string]$AliasName) {
    $normTarget = $TargetDir.TrimEnd('\', '/')
    $layers = Get-WindowsPathLayers

    # Windows searches Machine (System) PATH entries before User PATH entries
    $orderedDirs = @()
    foreach ($d in $layers.Machine) {
        $orderedDirs += [pscustomobject]@{ Dir = $d; Scope = "System PATH" }
    }
    foreach ($d in $layers.User) {
        $orderedDirs += [pscustomobject]@{ Dir = $d; Scope = "User PATH" }
    }

    foreach ($cmd in @("cgx_glfw.exe", $AliasName)) {
        $targetExe = Join-Path $TargetDir $cmd
        if (-not (Test-Path $targetExe)) {
            continue
        }

        foreach ($entry in $orderedDirs) {
            $normDir = $entry.Dir.TrimEnd('\', '/')
            if (-not $normDir -or -not (Test-Path $entry.Dir)) {
                continue
            }
            $candidate = Join-Path $entry.Dir $cmd
            if (Test-Path $candidate) {
                if ($normDir -ine $normTarget) {
                    $item = Get-Item $candidate -Force -ErrorAction SilentlyContinue
                    $ver = Get-CgxBinaryVersion $candidate
                    $verText = if ($ver) { "Version $ver, " } else { "" }
                    $modText = if ($item) { "modified $($item.LastWriteTime.ToString('yyyy-MM-dd HH:mm'))" } else { "" }
                    Write-Warn "PATH shadow detected for '$cmd':"
                    Write-Host "  Running '$cmd' from PATH will execute: $candidate (${verText}${modText})" -ForegroundColor Yellow
                    Write-Host "  Reason: Found earlier in $($entry.Scope) than $TargetDir." -ForegroundColor Yellow
                    Write-Host "  Fix: Uninstall the older installation, adjust PATH order, or run `"$targetExe`" directly." -ForegroundColor Gray
                }
                break
            }
        }
    }
}

# -----------------------------------------------------------------------------
# Native Binary Verification
# -----------------------------------------------------------------------------
function Test-InstalledBinary([string]$TargetDir) {
    Write-Step "Running native launch verification test..."
    $installedBin = Join-Path $TargetDir "cgx_glfw.exe"
    if (-not (Test-Path $installedBin)) {
        Write-Warn "Installed binary not found at $installedBin."
        return
    }

    $probe = Invoke-CgxProbe -ExePath $installedBin -Arguments @("--version") -TimeoutMs 5000
    $combined = ($probe.StdOut + "`n" + $probe.StdErr).Trim()
    if ($probe.TimedOut) {
        Write-Warn "Verification timed out while launching $installedBin --version."
        return
    }

    $ver = $null
    if ($combined -match 'Version\s+([0-9][^\r\n]*)') {
        $ver = $Matches[1].Trim()
    }

    if ($probe.ExitCode -eq 0) {
        $verInfo = if ($ver) { " (Version $ver)" } else { "" }
        Write-Success "CalculiX GraphiX binary launch verified${verInfo}."
    } elseif ($ver) {
        Write-Success "CalculiX GraphiX binary launch verified (Version $ver)."
    } else {
        Write-Warn "Launch verification exited with code $($probe.ExitCode): $combined"
    }

    if (Test-Path "test\test_vtu.fbl") {
        $vtuProbe = Invoke-CgxProbe -ExePath $installedBin -Arguments @("-bg", "test\test_vtu.fbl") -TimeoutMs 30000
        if ((-not $vtuProbe.TimedOut) -and ($vtuProbe.ExitCode -eq 0) -and (Test-Path "all.vtu")) {
            Write-Success "VTU export verification passed!"
            Remove-Item all.vtu, all.pvd, all_step_*.vtu -Force -ErrorAction SilentlyContinue
        } else {
            Write-Warn "VTU export smoke test did not produce all.vtu (exit code: $($vtuProbe.ExitCode))."
        }
    }
}

function Remove-FromUserPath([string]$TargetDir) {
    if ($SkipPath) {
        return
    }

    $normTarget = $TargetDir.Trim().TrimEnd('\', '/')
    $userRaw = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
    if ($userRaw) {
        $kept = @($userRaw -split ';' | Where-Object {
            (-not [string]::IsNullOrWhiteSpace($_)) -and ($_.Trim().TrimEnd('\', '/') -ine $normTarget)
        } | ForEach-Object { $_.Trim() })
        $newPath = $kept -join ';'
        if ($newPath -ne $userRaw) {
            [Environment]::SetEnvironmentVariable("Path", $newPath, [EnvironmentVariableTarget]::User)
            Write-Success "Removed $TargetDir from Windows User PATH."
        }
    }

    if ($env:Path) {
        $keptProc = @($env:Path -split ';' | Where-Object {
            (-not [string]::IsNullOrWhiteSpace($_)) -and ($_.Trim().TrimEnd('\', '/') -ine $normTarget)
        })
        $env:Path = $keptProc -join ';'
    }
}

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return [bool]$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Find-WindowsSetupInstalls {
    $regLocations = @(
        @{ Path = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{9F784F34-8A87-4C3F-982B-5E77114A1D9A}_is1"; Scope = "System" },
        @{ Path = "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{9F784F34-8A87-4C3F-982B-5E77114A1D9A}_is1"; Scope = "System" },
        @{ Path = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{9F784F34-8A87-4C3F-982B-5E77114A1D9A}_is1"; Scope = "User" }
    )

    $results = @()
    $seenExes = @()
    foreach ($loc in $regLocations) {
        if (-not (Test-Path $loc.Path)) {
            continue
        }
        try {
            $props = Get-ItemProperty -Path $loc.Path -ErrorAction Stop
            $rawUnins = [string]$props.UninstallString
            if ([string]::IsNullOrWhiteSpace($rawUnins)) {
                continue
            }
            $exePath = $rawUnins.Trim().Trim('"')
            if (-not (Test-Path $exePath)) {
                continue
            }
            $normExe = $exePath.ToLower()
            if ($seenExes -contains $normExe) {
                continue
            }
            $seenExes += $normExe

            $installLoc = [string]$props.InstallLocation
            if ([string]::IsNullOrWhiteSpace($installLoc)) {
                $installLoc = Split-Path -Parent $exePath
            } else {
                $installLoc = $installLoc.Trim().TrimEnd('\', '/')
            }

            $results += [pscustomobject]@{
                Scope                = $loc.Scope
                DisplayName          = [string]$props.DisplayName
                DisplayVersion       = [string]$props.DisplayVersion
                InstallLocation      = $installLoc
                ExePath              = $exePath
                UninstallString      = $rawUnins.Trim()
                QuietUninstallString = [string]$props.QuietUninstallString
            }
        } catch {
            # Ignore unreadable registry entries
        }
    }
    return $results
}

function Do-Uninstall([string]$RequestedInstallDir) {
    $isCustomDir = -not [string]::IsNullOrWhiteSpace($RequestedInstallDir)
    $defaultDir = Join-Path $env:USERPROFILE ".local\bin"
    $targetDir = Resolve-NormalizedPath $(if ($isCustomDir) { $RequestedInstallDir } else { $defaultDir })
    $sourceCacheDir = Join-Path $env:USERPROFILE ".cgx"

    Write-Step "Scanning for installed CalculiX GraphiX (GLFW Edition) components..."
    $foundBinaries = @(Get-CgxBinariesInDir $targetDir)
    $setupInstalls = @()
    $hasSourceCache = $false

    if (-not $isCustomDir) {
        $setupInstalls = @(Find-WindowsSetupInstalls)
        $hasSourceCache = Test-Path $sourceCacheDir
    }

    if (($foundBinaries.Count -eq 0) -and ($setupInstalls.Count -eq 0) -and (-not $hasSourceCache)) {
        Write-Warn "No CalculiX GraphiX (GLFW Edition) installation found in $targetDir."
        if ((-not $NonInteractive) -and (-not $isCustomDir)) {
            if (Confirm-User "Check a custom directory for cgx_glfw.exe?" "N") {
                $customRaw = Prompt-Value "Enter custom directory path" $defaultDir
                $targetDir = Resolve-NormalizedPath $customRaw
                $foundBinaries = @(Get-CgxBinariesInDir $targetDir)
            }
        }
        if ($foundBinaries.Count -eq 0) {
            Write-Host "Nothing to uninstall." -ForegroundColor Gray
            return
        }
    }

    if ($foundBinaries.Count -gt 0) {
        Write-Host "`nCLI binaries detected in ${targetDir}:" -ForegroundColor Cyan
        Write-CgxBinaryDetails $foundBinaries
    }

    if ($hasSourceCache) {
        Write-Host "`nBuild source cache detected:" -ForegroundColor Cyan
        Write-Host "  - $sourceCacheDir" -ForegroundColor Yellow
    }

    if ($setupInstalls.Count -gt 0) {
        Write-Host "`nWindows Setup installation(s) detected:" -ForegroundColor Cyan
        foreach ($s in $setupInstalls) {
            $verStr = if ($s.DisplayVersion) { " (Version $($s.DisplayVersion))" } else { "" }
            Write-Host "  - $($s.DisplayName)${verStr}: $($s.InstallLocation) [$($s.Scope)]" -ForegroundColor Yellow
        }
    }

    $removedAny = $false

    if ($foundBinaries.Count -gt 0) {
        if ($Force -or $NonInteractive -or (Confirm-User "Remove the detected CalculiX GraphiX binaries from ${targetDir}?" "N")) {
            $removedFromDir = $false
            foreach ($binPath in $foundBinaries) {
                if (Test-Path $binPath) {
                    Stop-LockingProcesses $binPath
                    $item = Get-Item $binPath -Force -ErrorAction SilentlyContinue
                    if ($item -and $item.IsReadOnly) {
                        $item.IsReadOnly = $false
                    }
                    Remove-Item -Path $binPath -Force
                    Write-Success "Removed: $binPath"
                    $removedFromDir = $true
                    $removedAny = $true
                }
            }

            if ($removedFromDir -and (-not $SkipPath)) {
                $layers = Get-WindowsPathLayers
                if (Test-PathEntryPresent $layers.User $targetDir) {
                    $remainingCount = @(Get-ChildItem -Path $targetDir -Force -ErrorAction SilentlyContinue).Count
                    if ($remainingCount -eq 0) {
                        if (Confirm-User "Directory $targetDir is now empty. Remove it from Windows User PATH?" "Y") {
                            Remove-FromUserPath $targetDir
                        }
                    } else {
                        Write-Host "Preserving $targetDir in User PATH because other files remain in that directory." -ForegroundColor Gray
                    }
                }
            }
        } else {
            Write-Host "Skipped removing binaries from $targetDir." -ForegroundColor Yellow
        }
    }

    if ($hasSourceCache -and (-not $KeepSource)) {
        if (Confirm-User "Remove build source cache (${sourceCacheDir})?" "Y") {
            Remove-Item -Path $sourceCacheDir -Recurse -Force
            Write-Success "Removed build source cache: $sourceCacheDir"
            $removedAny = $true
        } else {
            Write-Host "Preserved build source cache at $sourceCacheDir." -ForegroundColor Yellow
        }
    }

    if ($setupInstalls.Count -gt 0) {
        if ($NonInteractive) {
            foreach ($s in $setupInstalls) {
                Write-Host "[INFO] Windows Setup installation left untouched in non-interactive mode: $($s.InstallLocation)" -ForegroundColor Cyan
                if ($s.QuietUninstallString) {
                    Write-Host "       To uninstall silently: $($s.QuietUninstallString)" -ForegroundColor Gray
                }
            }
        } else {
            foreach ($s in $setupInstalls) {
                if (Confirm-User "Launch Windows Setup uninstaller for $($s.InstallLocation)?" "N") {
                    try {
                        $startParams = @{
                            FilePath = $s.ExePath
                            Wait     = $true
                            PassThru = $true
                        }
                        if ($s.Scope -eq "System" -and -not (Test-IsAdministrator)) {
                            $startParams["Verb"] = "RunAs"
                        }
                        $proc = Start-Process @startParams
                        if (-not (Test-Path $s.ExePath)) {
                            Write-Success "Removed Windows Setup installation: $($s.InstallLocation)"
                            $removedAny = $true
                        } elseif (($null -eq $proc.ExitCode) -or ($proc.ExitCode -eq 0)) {
                            Write-Host "Windows Setup uninstaller exited without removing $($s.InstallLocation)." -ForegroundColor Yellow
                        } else {
                            Write-Warn "Windows Setup uninstaller exited with code $($proc.ExitCode)."
                        }
                    } catch {
                        Write-Warn "Windows Setup uninstaller did not complete: $($_.Exception.Message)"
                    }
                }
            }
        }
    }

    if ($removedAny) {
        Write-Header "CalculiX GraphiX (GLFW Edition) - Uninstall Complete"
    } else {
        Write-Host "`nNo changes were made." -ForegroundColor Yellow
    }
}

# -----------------------------------------------------------------------------
# Main Installer Entrypoint
# -----------------------------------------------------------------------------
function Invoke-CgxInstaller {
    Write-Header "CalculiX GraphiX (GLFW Edition) - Windows Installer"

    if (-not [Environment]::Is64BitOperatingSystem) {
        Fail-Install "CalculiX GraphiX requires a 64-bit Windows operating system (x86_64)."
    }
    Write-Host "Detected Platform: Windows (x86_64 64-bit)" -ForegroundColor Green

    $selectedMode = $Mode
    if ($selectedMode -eq "auto") {
        if ($NonInteractive) {
            $selectedMode = "binary"
        } else {
            Write-Host "`nChoose an action:" -ForegroundColor Cyan
            Write-Host "  [1] Fast Install (Pre-built Release Binary) - Latest static release (cgx_glfw.exe + cgx.exe) [Default]"
            Write-Host "  [2] Build from Source (Stable Release Tag) - Compiles locally with MSYS2 UCRT64 (cgx_glfw.exe + cgx.exe)"
            Write-Host "  [3] Build from Source (Latest Development Commit) - Latest 'main' branch (cgx_glfw.exe + cgx_dev.exe)"
            Write-Host "  [4] Uninstall - Remove installed binaries, aliases, and PATH entries"
            $choice = Prompt-Value "`nEnter choice (1, 2, 3, or 4)" "1"

            switch ($choice) {
                "1" { $selectedMode = "binary" }
                "2" { $selectedMode = "build" }
                "3" { $selectedMode = "head" }
                "4" { $selectedMode = "uninstall" }
                default { Fail-Install "Invalid selection '$choice'. Expected 1, 2, 3, or 4." }
            }
        }
    }

    if ($selectedMode -eq "uninstall") {
        Do-Uninstall -RequestedInstallDir $InstallDir
        return
    }

    $defaultInstallDir = Join-Path $env:USERPROFILE ".local\bin"
    $chosenDir = $InstallDir
    if ([string]::IsNullOrWhiteSpace($chosenDir)) {
        if (-not $NonInteractive) {
            Write-Host "`nChoose install destination:" -ForegroundColor Cyan
            $chosenDir = Prompt-Value "Directory for binaries" $defaultInstallDir
        } else {
            $chosenDir = $defaultInstallDir
        }
    }
    $chosenDir = Resolve-NormalizedPath $chosenDir

    $isHead = ($selectedMode -eq "head")
    $aliasName = if ($isHead) { "cgx_dev.exe" } else { "cgx.exe" }

    if ($env:CGX_DOWNLOAD_BASE_URL) {
        $resolvedReleaseUrl = $env:CGX_DOWNLOAD_BASE_URL.TrimEnd('/')
    } elseif (-not [string]::IsNullOrWhiteSpace($Tag)) {
        $resolvedReleaseUrl = "https://github.com/${Repo}/releases/download/${Tag}"
    } else {
        $resolvedReleaseUrl = $DefaultReleaseUrl
    }

    if (-not (Test-ExistingInstallation -TargetDir $chosenDir -AliasName $aliasName)) {
        return
    }

    if ($selectedMode -eq "binary") {
        Do-FastInstall -TargetDir $chosenDir -AliasName $aliasName -BaseReleaseUrl $resolvedReleaseUrl -RequestedTag $Tag
    } elseif ($selectedMode -eq "build") {
        Do-BuildInstall -ForceHead $false -TargetDir $chosenDir -AliasName $aliasName -RequestedTag $Tag
    } elseif ($selectedMode -eq "head") {
        Do-BuildInstall -ForceHead $true -TargetDir $chosenDir -AliasName $aliasName -RequestedTag $Tag
    }

    Install-RuntimeTools
    Ensure-PathConfigured -TargetDir $chosenDir
    Test-InstalledBinary -TargetDir $chosenDir
    Test-PathShadowing -TargetDir $chosenDir -AliasName $aliasName

    $aliasCmd = if ($isHead) { "cgx_dev" } else { "cgx" }
    $modeLabel = if ($isHead) { "[Bleeding-Edge Development]" } else { "[Stable Release]" }

    Write-Header "CalculiX GraphiX (GLFW Edition) Installed! $modeLabel"
    Write-Host "`nInstalled Location:" -ForegroundColor Yellow
    Write-Host "   Directory : $chosenDir" -ForegroundColor White
    Write-Host "   Executable: $(Join-Path $chosenDir 'cgx_glfw.exe')" -ForegroundColor Green
    Write-Host "   Alias     : $(Join-Path $chosenDir $aliasName)" -ForegroundColor Green

    Write-Host "`nTo launch CGX from any Command Prompt or PowerShell:" -ForegroundColor Yellow
    Write-Host "   $aliasCmd model.frd            (open 3D finite element model)" -ForegroundColor Cyan
    Write-Host "   cgx_glfw model.frd        (open 3D finite element model)" -ForegroundColor Cyan
    Write-Host "   $aliasCmd                      (open interactive canvas)" -ForegroundColor Cyan
    Write-Host "   cgx_glfw -b script.fbl    (batch mode)" -ForegroundColor Cyan
    Write-Host "   cgx_glfw -bg script.fbl   (background batch mode)" -ForegroundColor Cyan

    Write-Host "`nDocumentation & Tutorials:" -ForegroundColor Yellow
    Write-Host "   https://github.com/$Repo" -ForegroundColor White
    Write-Host "================================================================`n" -ForegroundColor Cyan
}

Invoke-CgxInstaller
