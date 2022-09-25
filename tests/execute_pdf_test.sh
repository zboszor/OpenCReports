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
rm -f results/${TEST}.pdf.*.png results/${TEST}.asanout.*
./${TEST} 2>results/${TEST}.stderr >results/${TEST}.pdf

WARNINGS=
if [[ -f ${abs_srcdir}/expected/${TEST}.pdf ]]; then
	OUTEXP=$(ghostscript -dNOPAUSE -dBATCH -sDEVICE=png48 -r150 -sOutputFile=results/${TEST}.pdf.exp.%d.png ${abs_srcdir}/expected/${TEST}.pdf)
	OUTRES=$(ghostscript -dNOPAUSE -dBATCH -sDEVICE=png48 -r150 -sOutputFile=results/${TEST}.pdf.%d.png results/${TEST}.pdf)
	PAGESEXP=$(echo "$OUTEXP" | grep 'Processing pages .* through')
	PAGESEXPEND=$(echo "$PAGESEXP" | sed 's/^Processing pages .* through \([^\.]*\).*$/\1/')
	PAGESRES=$(echo "$OUTRES" | grep 'Processing pages .* through')
	PAGESRESEND=$(echo "$PAGESRES" | sed 's/^Processing pages .* through \([^\.]*\).*$/\1/')

	if [[ $PAGESEXPEND -ne $PAGESRESEND ]]; then
		OUTDIFF="Number of expected pages ($PAGESEXPEND) differ from the number of result pages ($PAGESRESEND)"
	else
		for i in $(seq 1 $PAGESEXPEND) ; do
			DIFF=$(compare -metric AE -fuzz '1%' results/${TEST}.pdf.exp.${i}.png results/${TEST}.pdf.${i}.png -colorspace RGB results/${TEST}.pdf.${i}.diff.png 2>&1)
			DIFF1=$(echo "$DIFF" | sed 's#^\([0-9]*\)compare:.*$#\1#')
			if [[ $DIFF1 != "0" ]]; then
				OUTDIFF="Page $i differs"
				break
			fi
			if [[ "$DIFF" != "$DIFF1" ]]; then
				if [[ -z $WARNINGS ]]; then
					WARNINGS="$DIFF"
				else
					WARNINGS="$WARNINGS\n$DIFF"
				fi
			fi
		done
	fi
else
	OUTDIFF="expected/${TEST}.pdf does not exist"
fi

ERRDIFF=$(diff -durpN ${abs_srcdir}/expected/${TEST}.stderr ${abs_builddir}/results/${TEST}.stderr 2>/dev/null)
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
