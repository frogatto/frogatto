#!/bin/bash

# script to remove all colorsync/image-calibration profiles from our PNGs, so they don't screw up our transparency (which is now respected by SDL, and wasn't before 1.2.14).

for i in `find images -name '*.png' -print`; do if sips -g profile $i | grep -iP 'profile: [^<]'; then sips -d profile $i; fi; done