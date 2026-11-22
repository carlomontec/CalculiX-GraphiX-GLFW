# ==============================================================================
# CalculiX GraphiX (GLFW Edition) - Windows PowerShell Universal Uninstaller
#
# 1-Liner Usage (from Windows PowerShell 5.1 or PowerShell 7):
#   irm https://raw.githubusercontent.com/carlomontec/CalculiX-GraphiX-GLFW/main/uninstall.ps1 | iex
#
# 1-Liner Environment Variables:
#   $env:CGX_INSTALL_DIR     = "$env:USERPROFILE\.local\bin"
#   $env:CGX_KEEP_SOURCE     = "1"      # Keep %USERPROFILE%\.cgx build cache
#   $env:CGX_FORCE           = "1"      # Uninstall without confirmation prompts
#   $env:CGX_NON_INTERACTIVE = "1"      # Run unattended without prompting
#   $env:CGX_SKIP_PATH       = "1"      # Do not modify User PATH
#
# Local Usage (from cloned repo):
#   powershell -ExecutionPolicy Bypass -File .\uninstall.ps1
#   .\uninstall.ps1 -NonInteractive
#   .\uninstall.ps1 -InstallDir "$env:USERPROFILE\bin" -Force
# ==============================================================================

[CmdletBinding()]
param (
    [Parameter(Mandatory=$false)]
    [string]$InstallDir = $(if ($env:CGX_INSTALL_DIR) { $env:CGX_INSTALL_DIR } else { "" }),

    [Parameter(Mandatory=$false)]
    [switch]$KeepSource,

    [Parameter(Mandatory=$false)]
    [switch]$Force,

    [Parameter(Mandatory=$false)]
    [switch]$NonInteractive,

    [Parameter(Mandatory=$false)]
    [switch]$SkipPath
)

$ErrorActionPreference = "Stop"

if (-not $KeepSource -and $env:CGX_KEEP_SOURCE -match '^(1|true|yes)$') { $KeepSource = $true }
if (-not $Force -and $env:CGX_FORCE -match '^(1|true|yes)$') { $Force = $true }
if (-not $NonInteractive -and $env:CGX_NON_INTERACTIVE -match '^(1|true|yes)$') { $NonInteractive = $true }
if (-not $SkipPath -and $env:CGX_SKIP_PATH -match '^(1|true|yes)$') { $SkipPath = $true }

$scriptPath = $MyInvocation.MyCommand.Path
$script:IsScriptFile = -not [string]::IsNullOrWhiteSpace($scriptPath)

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
# Path & Binary Inspection Helpers
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
        Fail-Install "Cannot remove $TargetPath while it is running. Close CalculiX GraphiX and re-run the uninstaller."
    }
}

# -----------------------------------------------------------------------------
# PATH Cleanup & Windows Setup Registry Discovery
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# Uninstall Execution
# -----------------------------------------------------------------------------
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

function Invoke-CgxUninstaller {
    Write-Header "CalculiX GraphiX (GLFW Edition) - Windows Uninstaller"
    Do-Uninstall -RequestedInstallDir $InstallDir
}

Invoke-CgxUninstaller
