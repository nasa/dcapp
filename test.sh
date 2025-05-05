#!/usr/bin/env bash

dcappHome="$(realpath $(dirname ${BASH_SOURCE[0]}))"
binDir="$dcappHome/pilotlight/out"

# cd $dcappHome/samples/test
# $binDir/dcapp-genheader samples/test/test.xml
# gcc -std=c++17 -g -O2 -fPIC -shared logic/logic.cpp -o logic/logic.so


cd $dcappHome/pilotlight/out
chmod +x ./pilot_light
./pilot_light -a dcapp
