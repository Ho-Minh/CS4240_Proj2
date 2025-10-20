param(
    [Parameter(Mandatory = $true)]
    [string] $Suite
)

$ErrorActionPreference = "Stop"

# Move to the script directory so relative paths work
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $scriptDir

$cppDir  = "..\cpp"
$suiteDir = Join-Path "..\public_test_cases" $Suite

if (!(Test-Path $suiteDir)) {
    Write-Host "Suite not found: $suiteDir" -ForegroundColor Red
    exit 1
}

$ir = Get-ChildItem $suiteDir -Filter *.ir | Select-Object -First 1
if ($null -eq $ir) {
    Write-Host "No .ir found in suite: $suiteDir" -ForegroundColor Red
    exit 1
}

$asmPath = Join-Path $cppDir ("{0}.s" -f $ir.BaseName)
if (!(Test-Path $asmPath)) {
    Write-Host "Missing asm: $asmPath (run build_all_public.sh first)" -ForegroundColor Yellow
    exit 1
}

Write-Host ("Testing suite: {0}" -f $Suite) -ForegroundColor Cyan
$allPass = $true

Get-ChildItem $suiteDir -Filter *.in | Sort-Object Name | ForEach-Object {
    $base = [System.IO.Path]::GetFileNameWithoutExtension($_.Name)
    $inFile = $_.FullName
    $expFile = Join-Path $suiteDir ("{0}.out" -f $base)
    $gotFile = Join-Path $scriptDir ("{0}_output_{1}.txt" -f $Suite, $base)

    Write-Host ("[{0} {1}] running..." -f $Suite, $base)
    & java -cp build main.java.mips.MIPSInterpreter --in $inFile $asmPath | Out-File -Encoding ascii $gotFile

    if (Test-Path $expFile) {
        $got = (Get-Content -Raw $gotFile).Replace("`r`n","`n")
        $exp = (Get-Content -Raw $expFile).Replace("`r`n","`n")
        if ($got -eq $exp -or (($got + "`n") -eq $exp) -or ($got -eq ($exp + "`n"))) {
            Write-Host ("[{0} {1}] PASS" -f $Suite, $base) -ForegroundColor Green
        } else {
            Write-Host ("[{0} {1}] FAIL - see: {2}" -f $Suite, $base, $gotFile) -ForegroundColor Red
            $allPass = $false
        }
    } else {
        Write-Host ("[{0} {1}] NO-EXP" -f $Suite, $base) -ForegroundColor Yellow
        $allPass = $false
    }
}

if ($allPass) {
    Write-Host "Suite passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Suite has failures." -ForegroundColor Red
    exit 1
}


