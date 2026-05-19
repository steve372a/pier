<# 
.SYNOPSIS
  Pier 元数据交互式编写工具 (PowerShell)
.DESCRIPTION
  按照 pier-repo-CONTRIBUTING.md 规范，交互式创建 metadata.sque 文件
  语言字符串位于 share/language/{lang}/langtools.ini
.NOTES
  要求: Windows PowerShell 5.1+ / PowerShell Core 6+
  用法: powershell -ExecutionPolicy Bypass -File mkmetadata.ps1 [-LangDir <path>]
  文件编码: UTF-8
#>

param(
    [string]$LangDir = ""
)
$OutputFile = "metadata.sque"
$OutDir = "."

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PierRoot = Split-Path -Parent $ScriptRoot
$Lang = @{}

# ============================================================
# 语言加载
# ============================================================

function Load-LangStrings {
    param([string]$FilePath)
    $table = @{}
    $currentKey = $null
    Get-Content -Path $FilePath -Encoding UTF8 | ForEach-Object {
        $line = $_.Trim()
        if ($line -match '^\[(.+)\]$') {
            $currentKey = $Matches[1]
        } elseif ($currentKey -and $line -ne '') {
            $table[$currentKey] = $line
            $currentKey = $null
        }
    }
    return $table
}

# 确定语言目录
if (-not $LangDir) {
    $langIni = Join-Path $PierRoot "etc\language.ini"
    if (Test-Path $langIni) {
        $langDirRaw = (Get-Content $langIni -Encoding UTF8 -TotalCount 1).Trim()
        $LangDir = $langDirRaw -replace '%PIER_ROOT%', $PierRoot
    }
}
if (-not $LangDir -or -not (Test-Path $LangDir)) {
    $LangDir = Join-Path $PierRoot "share\language\zh-CN"
}
$LangFile = Join-Path $LangDir "langtools.ini"
if (Test-Path $LangFile) {
    $Lang = Load-LangStrings -FilePath $LangFile
} else {
    Write-Warning "语言文件未找到: $LangFile，使用 zh-CN 默认值"
    $Lang = @{}
}

function Get-Str {
    param([string]$Key)
    if ($Lang.ContainsKey($Key)) { return $Lang[$Key] }
    return $Key
}

# ============================================================
# SHA256 哈希函数
# ============================================================

function Read-Required {
    param([string]$Key, [string]$Default)
    $prompt = Get-Str $Key
    $value = ""
    while ($value -eq "") {
        if ($Default) {
            $value = Read-Host -Prompt "$prompt [$Default]"
            if ($value -eq "") { $value = $Default }
        } else {
            $value = Read-Host -Prompt "$prompt"
        }
        if ($value -eq "") {
            Write-Host (Get-Str "required_msg") -ForegroundColor Yellow
        }
    }
    return $value
}

function Read-Optional {
    param([string]$Key, [string]$Default)
    $prompt = Get-Str $Key
    if ($Default) {
        $value = Read-Host -Prompt "$prompt [$Default]"
        if ($value -eq "") { $value = $Default }
    } else {
        $value = Read-Host -Prompt "$prompt$(Get-Str 'optional_suffix')"
    }
    return $value
}

function Read-Hash {
    Write-Host (Get-Str "prompt_hash")
    $pierHash = $null
    $searchPaths = @(
        Join-Path $PierRoot "pier-hash.exe"
        Join-Path $PierRoot "tools\pier-hash.exe"
        Join-Path $PierRoot "bin\pier-hash.exe"
    )
    foreach ($p in $searchPaths) {
        if (Test-Path $p) { $pierHash = $p; break }
    }
    if (-not $pierHash) {
        $pierHash = "pier-hash.exe"
    }
    while ($true) {
        $filePath = Read-Host
        $filePath = $filePath.Trim().Trim('"')
        if (-not (Test-Path $filePath)) {
            Write-Host (Get-Str "hash_file_not_found") -ForegroundColor Yellow
            continue
        }
        try {
            $output = & $pierHash -g $filePath 2>&1
            $allText = "$output"
            if ($allText -match '([0-9a-fA-F]{64})') {
                $hash = $Matches[1].ToLower()
                Write-Host ((Get-Str "hash_result") -f $hash) -ForegroundColor Green
                return $hash
            }
        } catch {
        }
        Write-Host (Get-Str "hash_failed") -ForegroundColor Red
    }
}

function Edit-Notepad {
    param([string]$FieldName, [string]$ExampleTemplate)
    $header = (Get-Str "notepad_template_header") -f $FieldName
    $template = @"
$header
==================
$ExampleTemplate
"@
    $tempFile = Join-Path $env:TEMP "pier_$([System.IO.Path]::GetRandomFileName()).txt"
    Set-Content -Path $tempFile -Value $template -Encoding UTF8
    Write-Host ((Get-Str "notepad_waiting") -f $FieldName)
    Start-Process notepad.exe -ArgumentList $tempFile -Wait
    $edited = Get-Content -Path $tempFile -Encoding UTF8 -Raw
    Remove-Item -Path $tempFile -Force
    $lines = $edited -split "`r`n|`n"
    $captured = $false
    $result = @()
    foreach ($line in $lines) {
        $trimmed = $line.Trim()
        if ($trimmed -eq '==================') {
            $captured = $true
            continue
        }
        if ($captured -and -not $trimmed.StartsWith('#')) {
            $result += $line
        }
    }
    $final = $result -join "`r`n"
    $final = $final.TrimEnd()
    if ($final -notmatch '::end$') {
        $final = $final + "`r`n::end"
        Write-Host (Get-Str "notepad_auto_end") -ForegroundColor Yellow
    }
    return $final
}

# ============================================================
# 开始
# ============================================================

Write-Host "========================================"
Write-Host (Get-Str "banner_name")
Write-Host "========================================"
Write-Host ""
Write-Host (Get-Str "intro_line1")
Write-Host (Get-Str "intro_line2")
Write-Host ""

# ============================================================
# 必填字段
# ============================================================

Write-Host (Get-Str "section_required") -ForegroundColor Cyan
Write-Host ""

$PkgName  = Read-Required "prompt_pkgname"
$Version  = Read-Required "prompt_version"
$OS       = Read-Required "prompt_os"

$DefaultInstaller = $PkgName.ToLower() -replace '[^a-z0-9-]', '-' -replace '--+', '-' -replace '^-', '' -replace '-$', ''
$Installer = Read-Required "prompt_installer" $DefaultInstaller

$DefaultURL = "$Installer-{version}.pie"
$URL          = Read-Required "prompt_url" $DefaultURL
$Profile      = Read-Required "prompt_profile"
$Distributor  = Read-Required "prompt_distributor"
$PackageSize  = Read-Required "prompt_pkgsize"
$Hash         = Read-Hash

Write-Host ""

# ============================================================
# 可选字段
# ============================================================

Write-Host (Get-Str "section_optional") -ForegroundColor Cyan
Write-Host ""

$Author    = Read-Optional "prompt_author"
$Notice    = Edit-Notepad "Notice" ""

Write-Host ""
$DefaultOpen = Edit-Notepad "DefaultOpen" "example.exe`r`nexample_*.exe`r`n::end"

Write-Host ""
Write-Host (Get-Str "prompt_alias_desc")
$Alias = Edit-Notepad "Alias" "example: example.exe `$1 -example `$2`r`n::end"

Write-Host ""
$OutDir   = Read-Optional "prompt_outdir" "."

Write-Host ""

# ============================================================
# 生成文件
# ============================================================

if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}

$OutPath = Join-Path $OutDir $OutputFile

$sb = [System.Text.StringBuilder]::new()

function Add-Field([string]$name, [string]$value) {
    [void]$sb.AppendLine("[$name]")
    [void]$sb.AppendLine($value)
    [void]$sb.AppendLine("")
}

function Add-OptionalField([string]$name, [string]$value) {
    if ($value) {
        [void]$sb.AppendLine("[$name]")
        [void]$sb.AppendLine($value)
        [void]$sb.AppendLine("")
    }
}

function Add-MultilineField([string]$name, [string]$value) {
    if ($value) {
        [void]$sb.AppendLine("[$name]")
        [void]$sb.AppendLine($value)
        [void]$sb.AppendLine("")
    }
}

Add-Field      "PackageName"    $PkgName
Add-Field      "Version"        $Version
Add-Field      "OS"             $OS
Add-Field      "InstallerName"  $Installer
Add-Field      "URL"            $URL
Add-Field      "ProFile"        $Profile
Add-OptionalField "Author"      $Author
Add-Field      "Distributor"    $Distributor
Add-OptionalField "Notice"      $Notice
Add-MultilineField "DefaultOpen" $DefaultOpen
Add-MultilineField "Alias"       $Alias
Add-Field      "HASH"           $Hash
Add-Field      "PackageSize"    $PackageSize
 
$content = $sb.ToString()

# 以 UTF-8 编码写入
Set-Content -Path $OutPath -Value $content -Encoding UTF8 -NoNewline

Write-Host "========================================"
Write-Host ((Get-Str "success_generated") -f $OutPath)
Write-Host "========================================"
Write-Host ""
Write-Host (Get-Str "preview_label")
Write-Host "---"
Write-Host $content
Write-Host "---"
Write-Host ""
Write-Host (Get-Str "next_steps")
Write-Host ((Get-Str "step1") -f $OutputFile)
Write-Host ((Get-Str "step2") -f $Installer.Substring(0,1), $Installer)
Write-Host (Get-Str "step3")