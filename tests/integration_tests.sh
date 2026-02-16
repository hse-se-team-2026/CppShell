#!/bin/bash
BIN="./bin/cppshell"

# Test 1: Simple pipe
echo "------------------------------------------------"
echo "Testing: echo 'hello pipeline' | cat"
RESULT=$(echo "echo 'hello pipeline' | cat" | $BIN)
if [[ "$RESULT" == *"hello pipeline"* ]]; then
  echo "✅ PASS"
else
  echo "❌ FAIL: Expected 'hello pipeline', got:"
  echo "$RESULT"
  exit 1
fi

# Test 2: Multiple pipes
echo "------------------------------------------------"
echo "Testing: echo 'one two three' | wc | cat"
# Output format of simple wc: lines words bytes
RESULT=$(echo "echo 'one two three' | wc | cat" | $BIN)
# Result should be "1 3 14" (possibly with prompt)
if echo "$RESULT" | grep -q "1 3 "; then
  echo "PASS"
else
  echo "FAIL: Expected to find ' 3 ' in output, got:"
  echo "$RESULT"
  exit 1
fi

# Test 3: Exit Code Propagation
echo "------------------------------------------------"
echo "Testing Exit Code: success | failure -> failure"
# Expect failure because the last command fails.
$BIN <<< "sh -c 'exit 0' | sh -c 'exit 1'" > /dev/null
CODE=$?
if [ $CODE -ne 0 ]; then
  echo "✅ PASS (Got $CODE)"
else
  echo "❌ FAIL: Expected non-zero exit code, got 0"
  exit 1
fi

echo "Testing Exit Code: failure | success -> success"
# Expect success because the last command succeeds.
$BIN <<< "sh -c 'exit 1' | sh -c 'exit 0'" > /dev/null
CODE=$?
if [ $CODE -eq 0 ]; then
  echo "✅ PASS (Got 0)"
else
  echo "❌ FAIL: Expected 0 exit code, got $CODE"
  exit 1
fi

# Test 4: Stderr Separation
echo "------------------------------------------------"
echo "Testing Stderr: ls /nonexistent | wc"
# Verify that:
# 1. Stderr contains the error message (is not redirecting to pipe).
# 2. Stdout (wc input) is empty because 'ls' failed and printed nothing to stdout.

OUT_FILE="out.tmp"
ERR_FILE="err.tmp"
$BIN <<< "ls /nonexistent_file_for_test | wc" > $OUT_FILE 2> $ERR_FILE

# Check stdout: should be "0 0 0" (empty input to wc)
if grep -E "0\s+0\s+0" $OUT_FILE > /dev/null; then
  echo "✅ PASS: Stdout is empty (correct)"
else
  echo "❌ FAIL: Stdout contained data (stderr leaked into pipe?):"
  cat $OUT_FILE
  rm $OUT_FILE $ERR_FILE
  exit 1
fi

# Check stderr: should contain error
if grep -q "ls:" $ERR_FILE || grep -q "No such file" $ERR_FILE; then
  echo "✅ PASS: Stderr contains error message"
else
  echo "❌ FAIL: Stderr was empty (error lost?):"
  cat $ERR_FILE
  rm $OUT_FILE $ERR_FILE
  exit 1
fi

rm $OUT_FILE $ERR_FILE

echo "------------------------------------------------"
# Test 5: Variable Assignment & Substitution
echo "------------------------------------------------"
echo "Testing Substitution: NAME=world -> echo \$NAME"
# Pass script as stdin
RESULT=$($BIN <<EOF
NAME=world
echo \$NAME
EOF
)
if echo "$RESULT" | grep -q "world"; then
  echo "✅ PASS (Simple Assignment)"
else
  echo "❌ FAIL: Expected 'world', got:"
  echo "$RESULT"
  exit 1
fi

# Test 6: Quoting Behavior
echo "------------------------------------------------"
echo "Testing Quoting: Double vs Single"
# Double quotes expand $NAME, single quotes do not.
RESULT=$($BIN <<EOF
NAME=universe
echo "Double: \$NAME"
echo 'Single: \$NAME'
EOF
)

if echo "$RESULT" | grep -q "Double: universe" && echo "$RESULT" | grep -q 'Single: $NAME'; then
  # Note: grep for '$NAME' is tricky. 'Single: $NAME' literal match.
  echo "✅ PASS (Quoting)"
else
  echo "❌ FAIL: Quoting mismatch. Output:"
  echo "$RESULT"
  exit 1
fi

# Test 7: Arithmetic Expansion
echo "------------------------------------------------"
echo "Testing Arithmetic: \$((10+32))"
RESULT=$(echo "echo \$((10+32))" | $BIN)

if echo "$RESULT" | grep -q "42"; then
  echo "✅ PASS (Arithmetic)"
else
  echo "❌ FAIL: Expected '42', got:"
  echo "$RESULT"
  exit 1
fi

# Test 7: Arithmetic Expansion
echo "------------------------------------------------"
echo "Testing Arithmetic: \$((10*3-30/3))"
RESULT=$(echo "echo \$((10*3-30/3))" | $BIN)

if echo "$RESULT" | grep -q "20"; then
  echo "✅ PASS (Arithmetic)"
else
  echo "❌ FAIL: Expected '20', got:"
  echo "$RESULT"
  exit 1
fi


# Test 9: Unknown Command -> xy
echo "------------------------------------------------"
echo "Testing Unknown Command: xy"
ERR_FILE="err_cmd.tmp"
$BIN <<< "xy" 2> $ERR_FILE > /dev/null
# Check exit code 127
CODE=$?
if [ $CODE -eq 127 ]; then
    echo "✅ PASS (Exit code 127)"
else
    echo "❌ FAIL: Expected 127 exit code, got $CODE"
    rm $ERR_FILE
    exit 1
fi

# Check stderr message
if grep -q "xy: command not found" $ERR_FILE; then
    echo "✅ PASS (Stderr message)"
else
    echo "❌ FAIL: Expected 'xy: command not found', got:"
    cat $ERR_FILE
    rm $ERR_FILE
    exit 1
fi
rm $ERR_FILE


echo "------------------------------------------------"
# Test 10: Grep Feature Tests
echo "------------------------------------------------"
echo "Testing Grep: Basic Match"
RESULT=$(echo "echo 'hello world' | grep 'world'" | $BIN)
if [[ "$RESULT" == *"hello world"* ]]; then
  echo "✅ PASS (Basic Match)"
else
  echo "❌ FAIL: Expected 'hello world', got:"
  echo "$RESULT"
  exit 1
fi

echo "Testing Grep: Case Insensitive (-i)"
RESULT=$(echo "echo 'HELLO WORLD' | grep -i 'world'" | $BIN)
if [[ "$RESULT" == *"HELLO WORLD"* ]]; then
  echo "✅ PASS (-i Match)"
else
  echo "❌ FAIL: Expected case-insensitive match, got:"
  echo "$RESULT"
  exit 1
fi

RESULT=$(echo "echo 'hello worldview' | grep -w 'world'" | $BIN)
# Check that the output does NOT contain "hello worldview"
if ! echo "$RESULT" | grep -q "hello worldview"; then
  echo "✅ PASS (-w No Match)"
else
  echo "❌ FAIL: Expected no match for 'worldview' with -w, got:"
  echo "$RESULT"
  exit 1
fi

RESULT=$(echo "echo 'hello world view' | grep -w 'world'" | $BIN)
if [[ "$RESULT" == *"hello world view"* ]]; then
  echo "✅ PASS (-w Match)"
else
  echo "❌ FAIL: Expected match for 'world' with -w, got:"
  echo "$RESULT"
  exit 1
fi

echo "Testing Grep: Context (-A)"
# We expect 2 lines: the match "line1" and the context "line2"
RESULT=$($BIN <<EOF
echo "line1\nline2\nline3" | grep -A 1 "line1"
EOF
)
if echo "$RESULT" | grep -q "line1" && echo "$RESULT" | grep -q "line2"; then
  echo "✅ PASS (-A Context)"
else
  echo "❌ FAIL: Expected match + context, got:"
  echo "$RESULT"
  exit 1
fi

echo "Testing Grep: Pipeline"
RESULT=$(echo "ls README.md | grep 'README'" | $BIN)
if [[ "$RESULT" == *"README.md"* ]]; then
  echo "✅ PASS (Pipeline Source)"
else
  echo "❌ FAIL: Expected 'README.md', got:"
  echo "$RESULT"
  exit 1
fi

echo "------------------------------------------------"
echo "All integration tests passed!"
exit 0

