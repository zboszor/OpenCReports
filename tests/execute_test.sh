#!/bin/bash

TEST=$1

abs_srcdir=${abs_srcdir:-$(pwd)}
abs_builddir=${abs_builddir:-$(pwd)}
top_srcdir=${top_srcdir:-$(dirname $(readlink -f "$0"))/..}

export abs_srcdir abs_builddir

if [[ ! -x ${TEST} ]]; then
	echo -e "[ \\033[1;31mERROR\\033[0;39m ] ${TEST} - does not exist"
	exit 0
fi

OCRPT_TEST=1
export OCRPT_TEST

mkdir -p ${abs_builddir}/results
rm -f results/${TEST}.stdout.*.png results/${TEST}.asanout.*
export ASAN_OPTIONS="log_path=results/${TEST}.asanout,fast_unwind_on_malloc=0"
export UBSAN_OPTIONS=print_stacktrace=true
export LSAN_OPTIONS="suppressions=${top_srcdir}/fontconfig.supp"
./${TEST} 2>results/${TEST}.stderr >results/${TEST}.stdout

if [[ -f ${abs_srcdir}/expected/${TEST}.stdout ]]; then
	OUTDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}.stdout ${abs_srcdir}/results/${TEST}.stdout 2>/dev/null)
	OUTRET=$?
	ERRDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}.stderr ${abs_srcdir}/results/${TEST}.stderr 2>/dev/null)
	ERRRET=$?
else
	FOUND=
	for i in ${abs_srcdir}/expected/${TEST}.stdout* ; do
		if [[ -f "$i" ]]; then
			SFX=${i#${abs_srcdir}/expected/${TEST}.stdout}
			OUTDIFF=$(diff -durpN "$i" ${abs_srcdir}/results/${TEST}.stdout 2>/dev/null)
			OUTRET=$?
			ERRDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}.stderr${SFX} ${abs_srcdir}/results/${TEST}.stderr 2>/dev/null)
			ERRRET=$?
			if [[ -z "$OUTDIFF" ]]; then
				FOUND=1
				break
			fi
		else
			OUTDIFF="${abs_srcdir}/expected/${TEST}.stdout or a variant does not exist."
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
