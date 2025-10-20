$ErrorActionPreference = "Stop"

# Move to the script directory so relative paths work
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $scriptDir

$asmPath = "..\cpp\prime.s"
$inDir   = "..\public_test_cases\prime"

Write-Host "Testing prime cases (0..9)" -ForegroundColor Cyan
$allPass = $true

for ($i = 0; $i -le 9; $i++) {
    $inFile   = Join-Path $inDir ("{0}.in" -f $i)
    $expFile  = Join-Path $inDir ("{0}.out" -f $i)
    $gotFile  = Join-Path $scriptDir ("output_{0}.txt" -f $i)

    Write-Host ("[prime {0}] running..." -f $i)

    # Run interpreter; write output with ASCII encoding to match expected files
    & java -cp build main.java.mips.MIPSInterpreter --in $inFile $asmPath | Out-File -Encoding ascii $gotFile

    $got = (Get-Content -Raw $gotFile).Replace("`r`n","`n")
    $exp = (Get-Content -Raw $expFile).Replace("`r`n","`n")

    if ($got -eq $exp) {
        Write-Host ("[prime {0}] PASS" -f $i) -ForegroundColor Green
    } else {
        Write-Host ("[prime {0}] FAIL - see: {1}" -f $i, $gotFile) -ForegroundColor Red
        $allPass = $false
    }
}

if ($allPass) {
    Write-Host "All prime cases passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some prime cases failed." -ForegroundColor Red
    exit 1
}


