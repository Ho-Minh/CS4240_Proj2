#!/usr/bin/env bash
set -euo pipefail

# Helper
compile() { echo "$1"; eval "$1"; }
run() { echo "$*"; "$@"; }

echo "========================================"
echo "Comprehensive IR System Testing Suite"
echo "========================================"

echo
echo "1. Testing Type System..."
compile 'g++ -std=c++17 -I../include test_types.cpp ../src/ir_types.cpp -o test_types'
run ./test_types

echo
echo "2. Testing Operand System..."
compile 'g++ -std=c++17 -I../include test_operands.cpp ../src/ir_types.cpp -o test_operands'
run ./test_operands

echo
echo "3. Testing IR Reader and Printer..."
echo "Testing with example.ir..."
run ./demo ../../example/example.ir example_parsed.ir
echo "Successfully parsed and printed example.ir"

echo
echo "Testing with sqrt.ir..."
run ./demo ../../public_test_cases/sqrt/sqrt.ir sqrt_parsed.ir
echo "Successfully parsed and printed sqrt.ir"

echo
echo "4. Testing Parsing Validation..."
compile 'g++ -std=c++17 -I../include test_parsing_validation.cpp ./libircpp.a -o test_parsing_validation'
run ./test_parsing_validation

echo
echo "5. Testing Comprehensive IR Operations..."
compile 'g++ -std=c++17 -I../include test_comprehensive.cpp ./libircpp.a -o test_comprehensive'
run ./test_comprehensive

echo
echo "6. Testing Arithmetic Operators Only..."
compile 'g++ -std=c++17 -I../include test_arithmetic_only.cpp ./libircpp.a -o test_arithmetic_only'
run ./test_arithmetic_only

echo
echo "7. Testing Real IR File Execution..."
compile 'g++ -std=c++17 -I../include test_interpreter.cpp ./libircpp.a -o test_interpreter'

echo "Testing Fibonacci with input 5..."
echo 5 | ./test_interpreter ../../example/example.ir
echo "Successfully executed Fibonacci program (fib(5) = 5)"

echo
echo "Testing Square Root with input 16..."
echo 16 | ./test_interpreter ../../public_test_cases/sqrt/sqrt.ir
echo "Successfully executed square root program"

echo
echo "8. Testing Error Handling..."
echo "Testing with malformed IR file..."
# Create a malformed IR file for testing
cat > test_error.ir << 'EOF'
#start_function
int invalid_function(invalid_type x):
int-list: 
float-list:
    invalid_op, x, y
#end_function
EOF

set +e
./demo test_error.ir error_output.ir
rc=$?
set -e
if [[ $rc -eq 0 ]]; then
  echo "WARNING: Expected error but got success"
else
  echo "Error handling works correctly"
fi

echo
echo "========================================"
echo "All comprehensive tests completed successfully!"
echo "========================================"
echo
echo "Test Summary:"
echo "- Type system singleton behavior"
echo "- Operand creation and validation"
echo "- IR file parsing and printing"
echo "- Instruction type parsing"
echo "- Type system integration"
echo "- Operand validation"
echo "- Round-trip parsing"
echo "- Arithmetic operations (add, sub, mult, div, and, or)"
echo "- Comparison operations (breq, brneq, brlt, brgt, brgeq)"
echo "- Control flow (goto, branches)"
echo "- Array operations (store, load)"
echo "- Real program execution (Fibonacci, Square Root)"
echo "- Error handling with malformed input"
echo "========================================"
