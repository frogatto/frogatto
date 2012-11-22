#!/bin/bash
#script that replaces the "land" variable of tileciv maps with the contents
#of a numeric matrix in a plaintext file
#USAGE: in the frogatto top-level folder, run
#utils/civ_import_land.sh data/level/civ/<level>.txt
TILES=`python utils/civ_import_land.py $1`;
sed -i '/^[[:space:]]*#/!s/land[ ]*=[ ]*\[[0-9,]*\]/'"$TILES"'/' ${1%.*}.cfg;

