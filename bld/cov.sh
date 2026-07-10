#!/bin/sh

set -e

INFO_PATH="/tmp/coverage.info" 
REP_PATH="/tmp/coverage-report"
SRC_PATH="src"

usage() {
	echo "usage: ${0} [executable|make target ...]"
	echo "executes all tests when no executables supplied"
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
	EXECS="${*}"
else
	for EXEC_C in tst/tst-*c; do
		EXECS="${EXECS} $(echo "${EXEC_C}" | sed -E 's/tst\/tst\-(.*)\.c/test-\1/g')"
	done
fi

for EXEC in ${EXECS}; do
	# dashes in test names are not tolerated
	TEST_NAME="$(echo "${EXEC}" | sed -E 's/-/_/g')"

	# execute test or executable to generate .gcda
	if [ "$(echo "${EXEC}" | cut -c 1-5)" = "test-" ]; then
		make \
			CC="gcc" \
			OFLAGS="-O0" \
			"${EXEC}"
	else
		"./${EXEC}"
	fi

	# generate coverage info for the individual test
	geninfo \
		--test-name "${TEST_NAME}" \
		--mcdc-coverage \
		--branch-coverage \
		--all \
		--no-external \
		--output-file "${INFO_PATH}/${EXEC}.info" \
		"${SRC_PATH}"

	# clear execution data
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

# TODO not finding the file

ONLY_HTML=$(find "${REP_PATH}" -name "${1}.c.gcov.html")

if [ $# -eq 1 ] && [ -f "${ONLY_HTML}" ]; then
	xdg-open "${ONLY_HTML}"
else
	xdg-open "${REP_PATH}/index.html"
fi

# clear .gnco for next (non-coverage) run
make clean all

