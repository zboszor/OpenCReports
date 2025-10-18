#!/bin/bash

TEST=$1

abs_srcdir=${abs_srcdir:-$(pwd)}
abs_builddir=${abs_builddir:-$(pwd)}
top_srcdir=${top_srcdir:-$(dirname $(readlink -f "$0"))/..}

export abs_srcdir abs_builddir

if [[ ! -f ${TEST}${TESTSFX} ]]; then
	echo -e "[ \\033[1;31mERROR\\033[0;39m ] ${TEST}${TESTSFX} - does not exist"
	exit 0
fi

OCRPT_TEST=1
export OCRPT_TEST

mkdir -p ${abs_builddir}/results
rm -f ${abs_builddir}/results/${TEST}${TESTSFX}.stdout.*.png ${abs_builddir}/results/${TEST}${TESTSFX}.asanout.*
export ASAN_OPTIONS="log_path=${abs_builddir}/results/${TEST}${TESTSFX}.asanout,fast_unwind_on_malloc=0"
export UBSAN_OPTIONS=print_stacktrace=true
export LSAN_OPTIONS="suppressions=${top_srcdir}/ignore.supp"
if [[ $TESTSFX == '.php' ]]; then
	php ${TEST}${TESTSFX} 2>${abs_builddir}/results/${TEST}${TESTSFX}.stderr >${abs_builddir}/results/${TEST}${TESTSFX}.stdout
else
	./${TEST}${TESTSFX} 2>${abs_builddir}/results/${TEST}${TESTSFX}.stderr >${abs_builddir}/results/${TEST}${TESTSFX}.stdout
fi

if [[ -f ${abs_srcdir}/expected/${TEST}${TESTSFX}.stdout ]]; then
	OUTDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}${TESTSFX}.stdout ${abs_builddir}/results/${TEST}${TESTSFX}.stdout 2>/dev/null)
	OUTRET=$?
	ERRDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}${TESTSFX}.stderr ${abs_builddir}/results/${TEST}${TESTSFX}.stderr 2>/dev/null)
	ERRRET=$?
else
	FOUND=
	for i in ${abs_srcdir}/expected/${TEST}${TESTSFX}.stdout* ; do
		if [[ -f "$i" ]]; then
			SFX=${i#${abs_srcdir}/expected/${TEST}${TESTSFX}.stdout}
			OUTDIFF=$(diff -durpN "$i" ${abs_builddir}/results/${TEST}${TESTSFX}.stdout 2>/dev/null)
			OUTRET=$?
			ERRDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}${TESTSFX}.stderr${SFX} ${abs_builddir}/results/${TEST}${TESTSFX}.stderr 2>/dev/null)
			ERRRET=$?
			if [[ -z "$OUTDIFF" ]]; then
				FOUND=1
				break
			fi
		else
			OUTDIFF="${abs_srcdir}/expected/${TEST}${TESTSFX}.stdout or a variant does not exist."
		fi
	done
fi

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
