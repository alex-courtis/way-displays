#!/bin/sh

FROM="${HOME}/src/alex-c-lib"
TO="lib/alex-c"

cp "${FROM}/LICENSE" "${FROM}/README.md" "${TO}"

mkdir -p "${TO}/inc" "${TO}/src" "${TO}/tst"

cp "${FROM}/inc/"*h "${TO}/inc"
cp "${FROM}/src/"*c "${TO}/src"
cp "${FROM}/tst/"*[ch] "${TO}/tst"

rm "${TO}/tst/tst-"*

