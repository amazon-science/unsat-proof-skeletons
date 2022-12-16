#!/bin/sh

cd cadical
make clean
cd ../drat-trim
make clean
cd ../
rm lrat-map lrat-skel
rm -r icad-proofs icad-logs icad-forms icad-skeletons icad-out icad-out-logs