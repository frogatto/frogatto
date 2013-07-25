#!/usr/bin/python
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
		#this'll break if there's more than one coin
		#defined in the same line. Hopefully never happens.
		
		for line in y[1]:
			if "\"gold_berry\"" in line: coins += 1
			if "\"coin_silver\"" in line: coins += 1
			if "\"coin_gold\"" in line: coins += 5
			if "\"coin_gold_big\"" in line: coins += 20
			if "\"coin_gold_enormous\"" in line: coins += 100
		print y[0] + ":", coins

if __name__ == "__main__":
	if len(sys.argv) > 1:
		levels = sys.argv[1:]
		main(levels)
