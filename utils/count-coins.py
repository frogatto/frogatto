#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, sys, codecs
#usage: from the module base folder:
#utils/count-coins.py data/level/<Ambient>/*

def shortname(filename):
	return filename.split('/')[-1]

def main(levels):
	levels = [(shortname(x), codecs.open(x, encoding="utf-8").readlines()) for x in levels]
	for y in levels:
		coins = 0
		for line in y[1]:
			coins += (line.count('"gold_berry"') * 1 +
			          line.count('"coin_silver"') * 1 +
			          line.count('"coin_gold"') * 5 +
			          line.count('"coin_gold_big"') * 20 +
			          line.count('"coin_gold_enormous"') * 100)
		print(y[0] + ": " + str(coins))

if __name__ == "__main__":
	if len(sys.argv) > 1:
		levels = sys.argv[1:]
		main(levels)
