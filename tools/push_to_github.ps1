<#
.SYNOPSIS
    把本地 master（工作区布局）快进推送到 GitHub。

.DESCRIPTION
    只做【快进】推送，绝不 force。推送前会校验 origin/master 是否为 master 的祖先，
    不满足就直接中止，避免意外重写远端历史。

    ⚠ 必须在 DSH 沙箱【之外】的普通 PowerShell 里运行。
      沙箱屏蔽了 schannel 的凭据访问，git 连不上 GitHub，会报：
        fatal: ... schannel: AcquireCredentialsHandle failed: SEC_E_NO_CREDENTIALS

.PARAMETER DryRun
    只做干跑（git push --dry-run），不真正推送。

.EXAMPLE
    .\tools\push_to_github.ps1 -DryRun
    .\tools\push_to_github.ps1
#>
param(
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

# 切到仓库根
$top = (git rev-parse --show-toplevel 2>$null)
if (-not $top) { throw "当前目录不是 git 仓库" }
Set-Location $top

Write-Host "== 1. 仓库根 ==" -ForegroundColor Cyan
Write-Host "   $top"

Write-Host "== 2. 当前 master 的目录结构 ==" -ForegroundColor Cyan
git ls-tree --name-only HEAD | ForEach-Object { Write-Host "   $_" }

Write-Host "== 3. 工作区状态 ==" -ForegroundColor Cyan
$st = git status --porcelain
if ($st) {
    Write-Host "   有未提交改动，请先提交：" -ForegroundColor Yellow
    $st | ForEach-Object { Write-Host "   $_" }
    throw "工作区不干净，已中止"
}
Write-Host "   干净 ✓" -ForegroundColor Green

Write-Host "== 4. 抓取远端 ==" -ForegroundColor Cyan
git fetch origin
if ($LASTEXITCODE -ne 0) {
    throw @"
fetch 失败。请先确认凭据可用：
  git config --global credential.helper manager
然后在任意 git 命令中按提示登录 GitHub（浏览器授权），或使用 Personal Access Token。
"@
}

Write-Host "== 5. 校验是否可快进 ==" -ForegroundColor Cyan
git merge-base --is-ancestor origin/master master
if ($LASTEXITCODE -ne 0) {
    Write-Host "   origin/master 不是 master 的祖先 → 推送会重写历史。" -ForegroundColor Red
    Write-Host "   已中止（本脚本不做 force）。如确需重写，请手动：git push --force origin master" -ForegroundColor Red
    throw "不可快进"
}
$n = git rev-list --count origin/master..master
Write-Host "   可快进，将推送 $n 个提交：" -ForegroundColor Green
git log --oneline origin/master..master | ForEach-Object { Write-Host "     $_" }

Write-Host "== 6. 干跑 ==" -ForegroundColor Cyan
git push --dry-run origin master
if ($LASTEXITCODE -ne 0) { throw "干跑失败，已中止" }

if ($DryRun) {
    Write-Host "== 指定了 -DryRun，停在干跑，未实际推送 ==" -ForegroundColor Yellow
    return
}

Write-Host "== 7. 正式推送 ==" -ForegroundColor Cyan
git push -u origin master
if ($LASTEXITCODE -ne 0) { throw "推送失败" }

Write-Host "== 8. 验证 ==" -ForegroundColor Cyan
Write-Host "   远端 master 的目录结构：" -ForegroundColor Gray
git ls-tree --name-only origin/master | ForEach-Object { Write-Host "     $_" }
Write-Host "   远端最近 3 次提交：" -ForegroundColor Gray
git log --oneline -3 origin/master | ForEach-Object { Write-Host "     $_" }
git status -sb | ForEach-Object { Write-Host "   $_" }

Write-Host ""
Write-Host "== 9. 最后一步（网页操作，脚本做不了）==" -ForegroundColor Yellow
Write-Host "   打开 https://github.com/KoishiLover/sdl3_engine/settings/branches"
Write-Host "   把 Default branch 从 main 改成 master"
Write-Host ""
Write-Host "   如需回滚远端 master（原始值 088b075）：" -ForegroundColor Gray
Write-Host "     git push --force origin 088b075:master"
