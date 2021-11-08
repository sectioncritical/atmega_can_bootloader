#!/bin/sh

rm -f boot-flow*.svg discovery.svg load.svg reboot.svg
plantuml -tsvg boot-flow.plantuml
mv boot-flow.svg discovery.svg
mv boot-flow_001.svg load.svg
mv boot-flow_002.svg reboot.svg
plantuml -tsvg boot-start.plantuml
dos2unix *.svg
