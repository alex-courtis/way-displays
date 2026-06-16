#!/bin/sh

FROM="${HOME}/src/alex-c-lib"
TO="lib/alex-c"

for l in fn ptable itable stable slist sset pset str; do
	cp -v "${FROM}/src/${l}.c" "${TO}/src"
	cp -v "${FROM}/inc/${l}.h" "${TO}/inc"
	if [ -f "${FROM}/tst/assert-${l}.h" ]; then
		cp -v "${FROM}/tst/assert-${l}.h" "${TO}/tst"
	fi
	if [ -f "${FROM}/tst/expect-${l}.h" ]; then
		cp -v "${FROM}/tst/expect-${l}.h" "${TO}/tst"
	fi
done

for t in asserts.h expects.h tst.h util-file.h util-file.c; do
	cp -v "${FROM}/tst/${t}" "${TO}/tst"
done

