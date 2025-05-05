#!/usr/bin/env bash

dcappHome="$(realpath $(dirname ${BASH_SOURCE[0]}))"

cd $dcappHome/pilotlight/out
./pilot_light -a dcapp $*
