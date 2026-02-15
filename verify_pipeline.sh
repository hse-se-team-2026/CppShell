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
echo "All integration tests passed!"
exit 0

