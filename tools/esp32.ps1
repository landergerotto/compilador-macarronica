# Executar no PowerShell na raiz do repositorio. Requer ESP-IDF instalado.
[CmdletBinding()]
param(
    [ValidateSet('generate', 'build', 'flash', 'monitor')]
    [string]$Action = 'build',
    [string]$Program = 'examples/esp32_blink.mac',
    [string]$Port = 'COM4',
    [string]$IdfPath = $env:IDF_PATH
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

function Invoke-Checked([string]$Command, [string[]]$Arguments) {
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Comando falhou ($LASTEXITCODE): $Command" }
}

# O ESP-IDF exige caminhos sem espacos. Usa o alias 8.3 do MESMO diretorio.
function Get-IdfCompatiblePath([string]$Path) {
    if ($Path -notmatch ' ') { return $Path }
    if (-not ('Macaronica.NativePath' -as [type])) {
        Add-Type -TypeDefinition 'using System; using System.Text; using System.Runtime.InteropServices; namespace Macaronica { public static class NativePath { [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)] public static extern uint GetShortPathName(string path, StringBuilder buffer, uint size); } }'
    }
    $buffer = New-Object System.Text.StringBuilder 32768
    $count = [Macaronica.NativePath]::GetShortPathName($Path, $buffer, 32768)
    if ($count -eq 0 -or $buffer.ToString() -match ' ') {
        throw 'ESP-IDF precisa de caminho sem espacos. Use uma pasta sem espacos ou habilite nomes curtos 8.3 para o projeto.'
    }
    return $buffer.ToString()
}

Push-Location $repoRoot
try {
    $shortRoot = Get-IdfCompatiblePath $repoRoot
    if ($Action -ne 'monitor') {
        foreach ($toolDirectory in @('C:\msys64\ucrt64\bin', 'C:\mingw64\bin', 'C:\msys64\usr\bin')) {
            if (Test-Path -LiteralPath $toolDirectory) { $env:Path = "$toolDirectory;$env:Path" }
        }
        $makeCommand = Get-Command mingw32-make -ErrorAction SilentlyContinue
        if (-not $makeCommand) { $makeCommand = Get-Command make -ErrorAction Stop }
        Invoke-Checked $makeCommand.Source @('release')
        $sourcePath = (Resolve-Path -LiteralPath $Program).Path
        $generatedDirectory = Join-Path $repoRoot 'build/esp32-generated'
        New-Item -ItemType Directory -Force $generatedDirectory | Out-Null
        $generatedPath = Join-Path $generatedDirectory 'prog_gerado.c'
        Invoke-Checked (Join-Path $repoRoot 'build/release/macaronica.exe') @('--alvo=esp32', $sourcePath, '-o', $generatedPath)
        [ordered]@{
            source = $sourcePath
            source_sha256 = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash
            generated_c_sha256 = (Get-FileHash -LiteralPath $generatedPath -Algorithm SHA256).Hash
            compiler_sha256 = (Get-FileHash -LiteralPath (Join-Path $repoRoot 'build/release/macaronica.exe') -Algorithm SHA256).Hash
        } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $generatedDirectory 'proveniencia.json') -Encoding UTF8
        if ($Action -eq 'generate') { return }
    }
    if (-not $IdfPath -and (Test-Path 'C:/Espressif/frameworks/esp-idf-v5.5.2/tools/idf.py')) {
        $IdfPath = 'C:/Espressif/frameworks/esp-idf-v5.5.2'
    }
    if (-not $IdfPath -or -not (Test-Path -LiteralPath (Join-Path $IdfPath 'export.ps1'))) {
        throw 'ESP-IDF nao encontrado. Ative o ambiente ESP-IDF ou informe -IdfPath C:\caminho\esp-idf.'
    }
    if (-not $env:IDF_TOOLS_PATH -and (Test-Path 'C:/Espressif/idf-env.json')) {
        $env:IDF_TOOLS_PATH = 'C:\Espressif'
    }
    . (Join-Path $IdfPath 'export.ps1')
    if (-not $env:IDF_PYTHON_ENV_PATH) { throw 'Falha ao ativar o Python do ESP-IDF.' }
    $idfPython = Join-Path $env:IDF_PYTHON_ENV_PATH 'Scripts/python.exe'
    $idfScript = Join-Path $IdfPath 'tools/idf.py'
    $projectPath = Join-Path $shortRoot 'esp32'
    $buildPath = Join-Path $shortRoot 'build/esp32'
    if ($Action -eq 'monitor') {
        Invoke-Checked $idfPython @($idfScript, '-C', $projectPath, '-B', $buildPath, '-p', $Port, 'monitor')
        return
    }
    New-Item -ItemType Directory -Force $buildPath | Out-Null
    $configPath = Join-Path $buildPath 'sdkconfig'
    if ((Test-Path -LiteralPath $configPath) -and
        -not (Select-String -LiteralPath $configPath -Pattern '^CONFIG_IDF_TARGET="esp32"$' -Quiet)) {
        throw 'O build existente usa outro alvo. Preserve-o e escolha um diretorio de build para esp32.'
    }
    Invoke-Checked $idfPython @($idfScript, '-C', $projectPath, '-B', $buildPath, '-D', "SDKCONFIG=$configPath", '-D', 'IDF_TARGET=esp32', 'build')
    if ($Action -eq 'flash') {
        Invoke-Checked $idfPython @($idfScript, '-C', $projectPath, '-B', $buildPath, '-p', $Port, '-b', '115200', 'flash')
    }
} finally {
    Pop-Location
}
