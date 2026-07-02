#!/bin/sh

set -e

INFO_PATH="/tmp/coverage.info" 
REP_PATH="/tmp/coverage-report"
SRC_PATH="src"

usage() {
	echo "usage: ${0} [test-x target ...]"
	exit 0
}

if [ "${1}" = "-h" ]; then
	usage
fi

rm -rf "${REP_PATH}"
rm -rf "${INFO_PATH}"
mkdir "${INFO_PATH}"

export LDLIBS="-lgcov"

make clean

# build with coverage flag to generate only desired .gcno
make \
	CC="gcc" \
	OFLAGS="-O0" \
	COVCFLAGS="-fprofile-arcs -ftest-coverage -fcondition-coverage" \
	all

if [ $# -gt 0 ]; then
	TESTS="${*}"
else
	for TEST_C in tst/tst-*c; do
		TESTS="${TESTS} $(echo "${TEST_C}" | sed -E 's/tst\/tst\-(.*)\.c/\1/g')"
	done
fi

for TEST in ${TESTS}; do

	# execute test target to generate .gcda
	make \
		CC="gcc" \
		OFLAGS="-O0" \
		"test-${TEST}"

	# generate coverage info for the individual test
	geninfo \
		--test-name "tst_${TEST}" \
		--mcdc-coverage \
		--branch-coverage \
		--all \
		--no-external \
		--output-file "${INFO_PATH}/${TEST}.info" \
		"${SRC_PATH}"

	# clear test execution data
	find . -name '*gcda' -delete -print

done

# combined report for all coverage info
genhtml \
	--show-details \
	--mcdc-coverage \
	--branch-coverage \
	--show-proportion \
	--dark-mode \
	--num-spaces 4 \
	--flat \
	--rc genhtml_hi_limit=85 \
	--rc genhtml_med_limit=60 \
	--output-directory "${REP_PATH}" \
	${INFO_PATH}

ONLY_HTML=$(find "${REP_PATH}" -name "${1}.c.gcov.html")

if [ $# -eq 1 ] && [ -f "${ONLY_HTML}" ]; then
	xdg-open "${ONLY_HTML}"
else
	xdg-open "${REP_PATH}/index.html"
fi

# clear .gnco for next (non-coverage) run
make clean all

