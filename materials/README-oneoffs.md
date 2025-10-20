## One-off commands and scripts

This cheatsheet lists quick, one-off commands to build IR → MIPS32 and run tests.

Notes
- Build: use the bash script (works on Ubuntu/macOS; WSL on Windows).
- Tests: use the PowerShell scripts (on Windows PowerShell/Core).

### Build (IR → MIPS32)

- Build all public test case IRs into `.s` files (Linux/macOS/WSL):

```bash
cd materials/mips-interpreter
bash ./build_all_public.sh
```

- Build a single IR into `.s` manually (if needed):

```bash
cd materials/cpp
make
./bin/ir_to_mips ../public_test_cases/<suite>/<file>.ir <file>.s
```

Examples:
```bash
# Prime
./bin/ir_to_mips ../public_test_cases/prime/prime.ir prime.s

# Quicksort
./bin/ir_to_mips ../public_test_cases/quicksort/quicksort.ir quicksort.s
```

### Test (run interpreter and compare)

- Run all public suites and compare outputs (Windows PowerShell):

```powershell
cd materials/mips-interpreter
powershell -ExecutionPolicy Bypass -File .\test_all_public.ps1
```

- Run only prime suite (0..9):

```powershell
cd materials/mips-interpreter
powershell -ExecutionPolicy Bypass -File .\test_prime.ps1
```

- Run only quicksort suite (0..9):

```powershell
cd materials/mips-interpreter
powershell -ExecutionPolicy Bypass -File .\test_quicksort.ps1
```

### Run a single case manually

- Prime case 0:
```powershell
cd materials/mips-interpreter
java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/prime/0.in ../cpp/prime.s
```

- Quicksort case 0:
```powershell
cd materials/mips-interpreter
java -cp build main.java.mips.MIPSInterpreter --in ../public_test_cases/quicksort/0.in ../cpp/quicksort.s
```

### Debug a single case

```powershell
cd materials/mips-interpreter
java -cp build main.java.mips.MIPSInterpreter --debug --in <input> <program.s>

# Examples
java -cp build main.java.mips.MIPSInterpreter --debug --in ../public_test_cases/prime/0.in ../cpp/prime.s
java -cp build main.java.mips.MIPSInterpreter --debug --in ../public_test_cases/quicksort/0.in ../cpp/quicksort.s
```

Debugger tips
- Press Enter to step; `p $reg` to print a register; `g <label>` to run to a label.
- Use `x/n offset($reg)` to inspect memory, e.g., `x/5 0($sp)`.

### Compare output manually

Windows (PowerShell):
```powershell
fc ..\public_test_cases\prime\0.out .\output_0.txt
```

Linux/macOS:
```bash
diff -u ../public_test_cases/prime/0.out ./output_0.txt
```


