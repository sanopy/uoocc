#!/bin/sh

total=0

unit_test() {
  echo $1 | ./cc.out > test.s
  gcc test.s -o test.out
  ./test.out
  actual=$?
  expected=$2
  if [ $actual = $expected ]; then
    echo "Test $total: Passed"
  else
    echo "Test $total: Failed"
    echo "\tExpected: $expected, Actual: $actual"
  fi
  total=`echo "$total+1" | bc`
  rm -f test.s test.out
}

unit_test 0 0
unit_test 2 2
unit_test 9 9
