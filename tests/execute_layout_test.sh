#!/bin/bash

function compare_pdf () {
	local EXPECTED_PDF
	EXPECTED_PDF=$(basename "$1")
	OUTDIFF=
	WARNINGS=
	OUTEXP=$(ghostscript -dNOPAUSE -dBATCH -sDEVICE=png48 -r150 -sOutputFile="${abs_builddir}/results/${EXPECTED_PDF}.exp.%d.png" "$1")
	OUTRES=$(ghostscript -dNOPAUSE -dBATCH -sDEVICE=png48 -r150 -sOutputFile="${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}.%d.png" "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}")
	PAGESEXP=$(echo "$OUTEXP" | grep 'Processing pages .* through')
	PAGESEXPEND=$(echo "$PAGESEXP" | sed 's/^Processing pages .* through \([^\.]*\).*$/\1/')
	PAGESRES=$(echo "$OUTRES" | grep 'Processing pages .* through')
	PAGESRESEND=$(echo "$PAGESRES" | sed 's/^Processing pages .* through \([^\.]*\).*$/\1/')

	if [[ $PAGESEXPEND -ne $PAGESRESEND ]]; then
		OUTDIFF="Number of expected pages ($PAGESEXPEND) differ from the number of result pages ($PAGESRESEND)"
	else
		for i in $(seq 1 "$PAGESEXPEND") ; do
			DIFF=$(compare -metric AE -fuzz '1%' "${abs_builddir}/results/${EXPECTED_PDF}.exp.${i}.png" "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}.${i}.png" -colorspace RGB "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}.${i}.diff.png" 2>&1)
			DIFF1=$(echo "$DIFF" | sed 's#^\([0-9]*\)compare:.*$#\1#')
			DIFF2=$(echo "$DIFF" | sed 's#^\([0-9]*\) .*$#\1#')
			if [[ $DIFF1 = "0" ]] || [[ $DIFF2 = "0" ]] ; then
				:
			else
				OUTDIFF="Page $i differs: $DIFF"
				break
			fi
			if [[ "$DIFF" = "$DIFF1" ]] || [[ "$DIFF" = "$DIFF2" ]]; then
				:
			else
				if [[ -z $WARNINGS ]]; then
					WARNINGS="$DIFF"
				else
					WARNINGS="$WARNINGS\n$DIFF"
				fi
			fi
		done
	fi
}

TEST=$1
TYPE=$2
TYPE=${TYPE:-1}

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

SFX=("unknown" "pdf" "html" "txt" "csv" "xml" "json")

mkdir -p "${abs_builddir}"/results
rm -f "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}".*.png
rm -f "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}".asanout.*

export ASAN_OPTIONS="log_path=${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}.asanout,fast_unwind_on_malloc=0"
export UBSAN_OPTIONS=print_stacktrace=true
export LSAN_OPTIONS="suppressions=${top_srcdir}/ignore.supp"

if [[ $TESTSFX == '.php' ]]; then
	php "${TEST}${TESTSFX}" "${TYPE}" 2>"${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}.stderr" >"${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}"
else
	"./${TEST}${TESTSFX}" "${TYPE}" 2>"${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}.stderr" >"${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}"
fi

if [[ -f "${abs_srcdir}/expected/${TEST}${TESTSFX}.${SFX[$TYPE]}" ]]; then
	EXPECTED_TEST="${abs_srcdir}/expected/${TEST}${TESTSFX}.${SFX[$TYPE]}"
	if [[ $TYPE = 1 ]]; then
		compare_pdf "$EXPECTED_TEST"
	else
		OUTDIFF=$(diff -durpN "$EXPECTED_TEST" "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}" 2>/dev/null)
	fi
else
	FOUND=
	for EXPECTED_TEST in "${abs_srcdir}/expected/${TEST}${TESTSFX}.${SFX[$TYPE]}"* ; do
		if [[ -f "$EXPECTED_TEST" ]]; then
			[[ "$EXPECTED_TEST" =~ \.stderr$ ]] && continue
			if [[ $TYPE = 1 ]]; then
				compare_pdf "$EXPECTED_TEST"
			else
				OUTDIFF=$(diff -durpN "$EXPECTED_TEST" "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}" 2>/dev/null)
			fi
			if [[ -z $OUTDIFF ]]; then
				FOUND=1
				break
			fi
		else
			OUTDIFF="${abs_srcdir}/expected/${TEST}${TESTSFX}.${SFX[$TYPE]} or a variant does not exist"
		fi
	done
	if [[ -z "$FOUND" ]]; then
		OUTDIFF="No matching expected result found"
	fi
fi

ERRDIFF=$(diff -durpN "${EXPECTED_TEST}.stderr" "${abs_builddir}/results/${TEST}${TESTSFX}.${SFX[$TYPE]}.stderr" 2>/dev/null)
ERRRET=$?

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
	[[ -n "$WARNINGS" ]] && echo -e "$WARNINGS"
fi

exit 0
