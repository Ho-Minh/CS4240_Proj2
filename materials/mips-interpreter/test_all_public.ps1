$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $scriptDir

$cppDir   = Join-Path $scriptDir "..\cpp"
$casesDir = Join-Path $scriptDir "..\public_test_cases"

if (!(Test-Path $casesDir)) {
    Write-Host "[ERROR] public_test_cases not found at: $casesDir" -ForegroundColor Red
    exit 1
}

Write-Host "Running all public test cases..." -ForegroundColor Cyan
$allPass = $true

# For each subfolder in public_test_cases
$suites = @(Get-ChildItem -Path $casesDir -Directory)
foreach ($suiteEntry in $suites) {
    $caseDir = $suiteEntry.FullName
    $name = $suiteEntry.Name
    # Determine asm by the .ir filename inside the suite
    $ir = Get-ChildItem -Path $caseDir -Filter *.ir | Select-Object -First 1
    if ($null -eq $ir) {
        Write-Host ("[WARN] No .ir found for {0}" -f $name) -ForegroundColor Yellow
        $allPass = $false
        return
    }
    $asmPath = Join-Path $cppDir ("{0}.s" -f $ir.BaseName)
    if (!(Test-Path $asmPath)) {
        Write-Host ("[WARN] Missing asm for {0}: {1}" -f $name, $asmPath) -ForegroundColor Yellow
        $allPass = $false
        return
    }

    Write-Host ("Testing suite: {0}" -f $name) -ForegroundColor Cyan
    # Find all .in files
    $inputs = @(Get-ChildItem -Path $caseDir -Filter *.in | Sort-Object Name)
    foreach ($inFile in $inputs) {
        $base = [System.IO.Path]::GetFileNameWithoutExtension($inFile.Name)
        $expFile = Join-Path $caseDir ("{0}.out" -f $base)
        $gotFile = Join-Path $scriptDir ("{0}_output_{1}.txt" -f $name, $base)
        Write-Host ("[{0} {1}] running..." -f $name, $base)
        try {
            & java -cp build main.java.mips.MIPSInterpreter --in $inFile.FullName $asmPath | Out-File -Encoding ascii $gotFile
        } catch {
            Set-Content -Path $gotFile -Encoding ascii -NoNewline -Value ""
        }

        if (Test-Path $expFile) {
            $got = ""
            if (Test-Path $gotFile) {
                $got = Get-Content -Raw $gotFile
                if ($null -eq $got) { $got = "" }
                $got = $got.Replace("`r`n","`n")
            }
            $exp = ""
            if (Test-Path $expFile) {
                $exp = Get-Content -Raw $expFile
                if ($null -eq $exp) { $exp = "" }
                $exp = $exp.Replace("`r`n","`n")
            }
            if ($got -eq $exp -or (($got + "`n") -eq $exp) -or ($got -eq ($exp + "`n"))) {
                Write-Host ("[{0} {1}] PASS" -f $name, $base) -ForegroundColor Green
            } else {
                Write-Host ("[{0} {1}] FAIL - see: {2}" -f $name, $base, $gotFile) -ForegroundColor Red
                $allPass = $false
            }
        } else {
            Write-Host ("[{0} {1}] No expected .out found" -f $name, $base) -ForegroundColor Yellow
            $allPass = $false
        }
    }
}

if ($allPass) {
    Write-Host "All public test cases passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Some public test cases failed." -ForegroundColor Red
    exit 1
}


