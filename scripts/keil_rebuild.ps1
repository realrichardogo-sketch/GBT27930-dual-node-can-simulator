# tools/keil_rebuild.ps1 - STEP 07-C2 Owner-side ARMCC rebuild helper. UNTESTED (written in WSL,
# never executed). It only BUILDS: UV4 -r (rebuild all), fromelf --bin, SHA-256. It never
# downloads to a board and never starts a debug session.
# Usage (from the repository root, uVision closed):
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\keil_rebuild.ps1 -UV4 "C:\Keil_v5\UV4\UV4.exe"
param(
    [string]$UV4 = "C:\Keil_v5\UV4\UV4.exe"   # Keil default install path; pass -UV4 for another location
)
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path $UV4)) { Write-Host "UV4.exe not found: $UV4 (pass -UV4 <path>)"; exit 2 }
if (Get-Process -Name "UV4" -ErrorAction SilentlyContinue) { Write-Host "uVision is running: close it first."; exit 2 }
$FromElf = Join-Path (Split-Path -Parent (Split-Path -Parent $UV4)) "ARM\ARMCC\Bin\fromelf.exe"
$Targets = @(
    @{ Name = "G474";  Proj = "source\Project1_G474\MDK-ARM\Project1_G474.uvprojx";                 Target = "Project1_G474"; Out = "source\Project1_G474\MDK-ARM\Project1_G474" },
    @{ Name = "F103B"; Proj = "source\Project1_F103B_Receiver_CubeMX\MDK-ARM\Project1_F103.uvprojx"; Target = "Project1_F103"; Out = "source\Project1_F103B_Receiver_CubeMX\MDK-ARM\Project1_F103" }
)
$LogDir = Join-Path $Root "evidence\armcc"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$rc = 0
foreach ($t in $Targets) {
    $proj = Join-Path $Root $t.Proj
    $log = Join-Path $LogDir ("armcc_" + $t.Name + ".log")
    Write-Host "Rebuilding $($t.Name): $proj"
    $p = Start-Process -FilePath $UV4 -ArgumentList @("-r", "`"$proj`"", "-t", $t.Target, "-j0", "-o", "`"$log`"") -Wait -PassThru
    Write-Host "  UV4 exit code $($p.ExitCode) (0 = no errors/warnings, 1 = warnings, >1 = errors)"
    if ($p.ExitCode -gt 1) { $rc = 1 }
    $out = Join-Path $Root $t.Out
    $axf = Join-Path $out ($t.Target + ".axf")
    if (Test-Path $axf) {
        if (Test-Path $FromElf) { & $FromElf --bin --output (Join-Path $out ($t.Target + ".bin")) $axf }
        Get-ChildItem -Path $out -Include *.axf, *.hex, *.bin, *.map, *.htm -Recurse |
            Get-FileHash -Algorithm SHA256 | ForEach-Object { "{0}  {1}" -f $_.Hash.ToLower(), $_.Path } |
            Out-File -Append -Encoding ascii (Join-Path $LogDir "ARMCC_OUTPUT_SHA256.txt")
        Select-String -Path (Join-Path $out "*.map") -Pattern "g_hw_diag_" |
            Measure-Object | ForEach-Object { Write-Host "  map lines with g_hw_diag_: $($_.Count)" }
    } else { Write-Host "  no AXF produced"; $rc = 1 }
}
exit $rc
