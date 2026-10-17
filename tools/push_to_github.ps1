#Requires -Version 5.1
<#
.SYNOPSIS
    Fast-forward push local master (workspace layout) to GitHub.

.DESCRIPTION
    Fast-forward ONLY. This script never forces.
    It aborts if origin/master is not an ancestor of master.

    NOTE: run this OUTSIDE the DSH sandbox.
    The sandbox blocks schannel credentials, so git cannot reach GitHub there:
        fatal: ... schannel: AcquireCredentialsHandle failed: SEC_E_NO_CREDENTIALS

.PARAMETER DryRun
    Only run "git push --dry-run", do not really push.

.EXAMPLE
    .\tools\push_to_github.ps1 -DryRun
    .\tools\push_to_github.ps1
#>
param(
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Step([string]$t) { Write-Host ""; Write-Host "== $t ==" -ForegroundColor Cyan }
function Ok([string]$t)   { Write-Host "   $t" -ForegroundColor Green }
function Warn([string]$t) { Write-Host "   $t" -ForegroundColor Yellow }

$top = (git rev-parse --show-toplevel 2>$null)
if (-not $top) { throw "Not a git repository (git rev-parse --show-toplevel failed)." }
Set-Location $top

Step "1. Repository root"
Ok $top

Step "2. Layout of local HEAD"
git ls-tree --name-only HEAD | ForEach-Object { Write-Host "   $_" }

Step "3. Working tree"
$st = git status --porcelain
if ($st) {
    Warn "Working tree is NOT clean. Commit or stash first:"
    $st | ForEach-Object { Write-Host "   $_" }
    throw "Aborted: working tree not clean."
}
Ok "clean"

Step "4. Fetch origin"
git fetch origin
if ($LASTEXITCODE -ne 0) {
    throw "fetch failed. Check credentials: git config --global credential.helper manager"
}

Step "5. Check fast-forward"
git merge-base --is-ancestor origin/master master
if ($LASTEXITCODE -ne 0) {
    Warn "origin/master is NOT an ancestor of master."
    Warn "Pushing would rewrite remote history. Aborted (this script never forces)."
    throw "Not fast-forwardable."
}
$n = git rev-list --count origin/master..master
Ok "fast-forwardable, $n commit(s) to push:"
git log --oneline origin/master..master | ForEach-Object { Write-Host "     $_" }

Step "6. Dry run"
git push --dry-run origin master
if ($LASTEXITCODE -ne 0) { throw "dry-run failed." }

if ($DryRun) {
    Warn "-DryRun given: stopping before the real push."
    return
}

Step "7. Push"
git push -u origin master
if ($LASTEXITCODE -ne 0) { throw "push failed." }

Step "8. Verify remote"
Write-Host "   remote master tree:" -ForegroundColor Gray
git ls-tree --name-only origin/master | ForEach-Object { Write-Host "     $_" }
Write-Host "   remote master log:" -ForegroundColor Gray
git log --oneline -3 origin/master | ForEach-Object { Write-Host "     $_" }
git status -sb | ForEach-Object { Write-Host "   $_" }

Step "9. Manual step (web UI, cannot be scripted)"
Warn "Open  https://github.com/KoishiLover/sdl3_engine/settings/branches"
Warn "Set 'Default branch' from main to master."
Warn ""
Warn "Rollback anchor (previous remote master): 088b075"
Warn "  git push --force origin 088b075:master"
