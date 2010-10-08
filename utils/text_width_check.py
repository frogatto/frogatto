#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os, sys, codecs
#usage: from frogatto's base folder, run:
#utils/text_width_check.py po/(desired file(s))
global MAXWIDTH
MAXWIDTH = 350

def main(filenames):
	fontdata = codecs.open("data/fonts.cfg", encoding="utf-8").readlines()
	fontdata = [x.strip() for x in fontdata]
	charwidths = {}
	i = 0
	while i < len(fontdata):
		if fontdata[i] == "[font]":
			fontid = ""
			#found a font, must check if it's the desired one
			while fontdata[i] != "[chars]":
				if "id=" in fontdata[i]:
					fontid = fontdata[i]
				i += 1
			if not "default" in fontid:
				continue
			chars=[]
			while fontdata[i] != "[/font]":
				if "chars=" in fontdata[i]:
					chars = list(fontdata[i].split("=", 1)[1])
				elif "width=" in fontdata[i]:
					for x in chars:
						charwidths[x]=int(fontdata[i].split("=")[1])
				elif "rect" in fontdata[i]:
					width = fontdata[i].split('=')[1].split(',')
					width = int(width[2]) - int(width[0]) + 1
					for x in chars:
						charwidths[x]=width
				i += 1
		i+=1
	
	for x in filenames:
		if x.split('.')[-1] == "pot": check = "msgid"
		elif x.split('.')[-1] == "po": check = "msgstr"
		else: continue
		print "File: " + x
		f = codecs.open(x, encoding="utf-8").readlines()
		#start from the first message line, i. e., the first with a #:
		l = [x[:2] for x in f].index("#:")
		msgline = 0
		while l < len(f):
			if "#:" in f[l] and "#:" not in f[l-1]:
				msgline = l
			if check in f[l]:
				linewidth = checkwidth(getmessage(f[l]), charwidths)
				if linewidth > MAXWIDTH:
					printline(f, msgline, linewidth)
				if len(getmessage(f[l])) == 0:
					l += 1
					while l < len(f) and '"' in f[l] and f[l][0] != "m":
						linewidth = checkwidth(getmessage(f[l]), charwidths)
						if linewidth > MAXWIDTH:
							printline(f, msgline, linewidth)
						l += 1
			l += 1
		
def printline(f, start, width):
	line = start
	print str(width) + " pixels:"
	while len(f[line].strip()) > 0:
		print f[line].strip()
		line += 1
	print
	
def checkwidth(line, widths):
	result = 0
	for x in line:
		if x in widths.keys():
			result += widths[x] + 2
	return result - 2
			
def getmessage(line):
	if line[0] == "m":
		return line.replace('\\n','').replace('\r','').replace('\n','').split(' ',1)[1][1:-1]
	else:
		return line.replace('\\n','').replace('\r','').replace('\n','')[1:-1]
	
if __name__ == "__main__":
	if len(sys.argv) > 1:
		main(sys.argv[1:])

