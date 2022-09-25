#!/bin/bash

TEST=$1

abs_srcdir=${abs_srcdir:-$(pwd)}
abs_builddir=${abs_builddir:-$(pwd)}

if [[ ! -x ${TEST} ]]; then
	echo -e "[ \\033[1;31mERROR\\033[0;39m ] ${TEST} - does not exist"
	exit 0
fi

OCRPT_TEST=1
export OCRPT_TEST

mkdir -p ${abs_builddir}/results
rm -f results/${TEST}.stdout.*.png results/${TEST}.asanout.*
./${TEST} 2>results/${TEST}.stderr >results/${TEST}.stdout

OUTDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}.stdout ${abs_srcdir}/results/${TEST}.stdout 2>/dev/null)
OUTRET=$?
ERRDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}.stderr ${abs_srcdir}/results/${TEST}.stderr 2>/dev/null)
ERRRET=$?

if [[ $OUTRET -ne 0 ]]; then
	ERROR=1
fi
if [[ -n "$OUTDIFF" ]]; then
	ERROR=1
fi
if [[ $ERRRET -ne 0 ]]; then
	ERRERROR=1
fi
if [[ -n "$ERRDIFF" ]]; then
	ERRERROR=1
fi
if [[ $ERROR -eq 1 ]]; then
	echo -e "[ \\033[1;31mERROR\\033[0;39m ] ${TEST}"
	echo "---- OUTDIFF ----"
	echo "${OUTDIFF}"
elif [[ $ERRERROR -eq 1 ]]; then
	echo -e "[ \\033[1;33mWARNING\\033[0;39m ] ${TEST}"
	echo "---- ERRDIFF ----"
	echo "${ERRDIFF}"
else
	echo -e "[ \\033[38;05;34mOK\\033[0;39m ] ${TEST}"
fi
