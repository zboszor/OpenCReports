#!/bin/bash

TEST=$1

if [[ ! -x ${TEST} ]]; then
	echo -e "[ \\033[1;31mERROR\\033[0;39m ] ${TEST} - does not exist"
	exit 0
fi

./${TEST} 2>results/${TEST}.stderr >results/${TEST}.stdout

OUTDIFF=$(diff -durpN expected/${TEST}.stdout results/${TEST}.stdout 2>/dev/null)
OUTRET=$?
ERRDIFF=$(diff -durpN expected/${TEST}.stderr results/${TEST}.stderr 2>/dev/null)
ERRRET=$?

if [[ $OUTRET -ne 0 ]]; then
	ERROR=1
fi
if [[ -n "$OUTDIFF" ]]; then
	ERROR=1
fi
if [[ $ERRRET -ne 0 ]]; then
	ERROR=1
fi
if [[ -n "$ERRDIFF" ]]; then
	ERROR=1
fi
if [[ $ERROR -eq 1 ]]; then
	echo -e "[ \\033[1;31mERROR\\033[0;39m ] ${TEST}"
	echo "---- OUTDIFF ----"
	echo "${OUTDIFF}"
	echo "---- ERRDIFF ----"
	echo "${ERRDIFF}"
else
	echo -e "[ \\033[38;05;34mOK\\033[0;39m ] ${TEST}"
fi
