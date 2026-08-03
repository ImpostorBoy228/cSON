#!/bin/sh
cd "$(dirname "$0")"
BIN=../cSON
fail=0

run_case() {
    name=$1; eout=$2; eerr=$3; ecode=$4
    shift 4
    "$BIN" "$@" > /tmp/cson_out 2> /tmp/cson_err || code=$?
    code=${code:-0}
    if [ "$code" -ne "$ecode" ]; then
        echo "FAIL $name: exit $code != $ecode"; fail=1
    fi
    if ! diff -u "$eout" /tmp/cson_out > /dev/null; then
        echo "FAIL $name: stdout"; diff -u "$eout" /tmp/cson_out; fail=1
    fi
    if ! diff -u "$eerr" /tmp/cson_err > /dev/null; then
        echo "FAIL $name: stderr"; diff -u "$eerr" /tmp/cson_err; fail=1
    fi
    unset code
}

run_case basic basic.out basic.err 0 basic.son
run_case if if.out if.err 0 if.son
run_case escapes escapes.out escapes.err 0 escapes.son
run_case comments comments.out comments.err 0 comments.son
run_case orphan orphan.out orphan.err 0 orphan.son
run_case empty empty.out empty.err 0 empty.son
run_case missing missing.out missing.err 1 doesnotexist.son

if [ "$fail" -eq 0 ]; then echo "all tests passed"; else exit 1; fi
