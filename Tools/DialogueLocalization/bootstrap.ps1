param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")),
    [string]$BasePython = ""
)

$ErrorActionPreference = "Stop"
$envRoot = Join-Path $ProjectRoot "Intermediate/DialogueLocalization/PythonEnv"
$python = Join-Path $envRoot "Scripts/python.exe"
$ready = Join-Path $envRoot ".ready"

if ([string]::IsNullOrWhiteSpace($BasePython)) {
    $pythonCommands = @("python", "python3.14", "python3")
    foreach ($pythonCommandName in $pythonCommands) {
        $pythonCommand = Get-Command $pythonCommandName -ErrorAction SilentlyContinue
        if ($pythonCommand) {
            $candidateVersion = & $pythonCommand.Source --version 2>&1
            if ($candidateVersion -match '^Python 3\.14(?:\.\d+)?') {
                $BasePython = $pythonCommand.Source
                break
            }
        }
    }

    if ([string]::IsNullOrWhiteSpace($BasePython)) {
        $pythonCandidates = @(
            (Join-Path $env:LOCALAPPDATA "Python/pythoncore-3.14-64/python.exe"),
            (Join-Path $env:ProgramFiles "Python314/python.exe"),
            (Join-Path ${env:ProgramFiles(x86)} "Python314/python.exe")
        )
        foreach ($pythonCandidate in $pythonCandidates) {
            if (Test-Path $pythonCandidate) {
                $candidateVersion = & $pythonCandidate --version 2>&1
                if ($candidateVersion -match '^Python 3\.14(?:\.\d+)?') {
                    $BasePython = $pythonCandidate
                    break
                }
            }
        }
    }
}

if ([string]::IsNullOrWhiteSpace($BasePython)) {
    throw "Python 3.14 was not found on PATH. Pass -BasePython with the Python 3.14 executable path."
}

if (-not (Test-Path $BasePython)) {
    $resolvedBasePython = Get-Command $BasePython -ErrorAction SilentlyContinue
    if ($resolvedBasePython) {
        $BasePython = $resolvedBasePython.Source
    }
}
if (-not (Test-Path $BasePython)) {
    throw "Base Python executable does not exist: $BasePython"
}

$baseVersion = & $BasePython --version 2>&1
if ($baseVersion -notmatch '^Python 3\.14(?:\.\d+)?') {
    throw "DialogueLocalization requires Python 3.14, but received: $baseVersion"
}

if (Test-Path $ready) {
    Remove-Item -LiteralPath $ready -Force
}

$recreateEnvironment = $false
if (Test-Path $python) {
    $environmentVersion = & $python --version 2>&1
    $recreateEnvironment = $environmentVersion -notmatch '^Python 3\.14(?:\.\d+)?'
}
if ($recreateEnvironment -and (Test-Path $envRoot)) {
    Remove-Item -LiteralPath $envRoot -Recurse -Force
}
if (-not (Test-Path $python)) {
    & $BasePython -m venv $envRoot
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create Python virtual environment."
    }
}
& $python -m pip install --disable-pip-version-check --requirement (Join-Path $PSScriptRoot "requirements.lock.txt")
if ($LASTEXITCODE -ne 0) {
    throw "Failed to install locked Python dependencies."
}
& $python -c "import openpyxl; assert openpyxl.__version__ == '3.1.5'"
if ($LASTEXITCODE -ne 0) {
    throw "Python dependency smoke test failed."
}
New-Item -ItemType Directory -Force -Path (Split-Path $ready) | Out-Null
Set-Content -Path $ready -Value "openpyxl=3.1.5" -Encoding UTF8
Write-Host "DialogueLocalization Python environment is ready: $envRoot"
