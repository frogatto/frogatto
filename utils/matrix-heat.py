#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""args: <file to analyze> <optional int quality>
Plots player's position points. Requires matplotlib for output.
See end of file for more details."""

from matplotlib.pylab import *
from math import *
import re
import sys


# \[.+\]			->				removes tags
# .*src.*|.*ms.*	->				removes non-dst lines
# dst="(.*)"		->	[\1],		formats dsts to square-bracketed list
# \s|\n				->				removes whitespace
# (.*)				->	[\1]		make it a list

def main(rawFile, radius):
	border = [10,10]
	
	radius = int(radius)
	points = []
	
	shortFileName = "output"
	parseName = re.match(r".*/(.*).cfg", rawFile)
	if parseName is not None:
		shortFileName = parseName.group(1)
	else:
		print "Error parsing " + rawFile

	print "reading data",
	sys.stdout.flush()
	dot = 0
	for line in open(rawFile, 'r').readlines():
		dst = re.match(r".*dst=\"([0-9|-]*),([0-9|-]*)", line)
		dot += 1
		if dot %100000 == 0:
			print "\b.",
			sys.stdout.flush()
		if dst and dst.group(2):
			points.append([int(dst.group(1)), int(dst.group(2))])
	print ""

	def samplemat(dims, points, offset):
		"""Write points into dims."""
		ptSize = radius
		pts = len(points)
		aa = zeros(dims)
		percentage = 0
		for i in range(pts):
			pt = [points[i][0]+offset[0], points[i][1]+offset[1]]
			newPercent = round((i+1.0)/pts, 3)*100
			if percentage != newPercent:
				percentage = newPercent
				print "  Mapping point " + str(i+1) + " of " + str(pts) + ". (" + str(percentage) + "%)"
			for x in range(ptSize):
				tX = pt[0]+x-ptSize/2
				for y in range(ptSize):
					tY = pt[1]+y-ptSize/2
					#print "plotting against the goverment at: " + str([tX, tY])
					if 0 <= tX < dims[0] and 0 <= tY < dims[1] :
						aa[tX,tY] = floor(aa[tX, tY] + ceil(ptSize/2-math.hypot(tX-pt[0], tY-pt[1]), 0), 400)
						
		print "Scaling colours."
		sys.stdout.flush()
		newMap = zeros(dims)
		for x in range(len(aa)):
			for y in range(len(aa[x])):
				newMap[x,y] = math.pow(aa[x,y], 0.5)
		return newMap
		
	def ceil(a, b):
		if a >= b:
			return a
		return b	
	def floor(a, b):
		if a <= b:
			return a
		return b

	def strip(array, index):
		newList = []
		for element in array:
			newList.append(element[index])
		return newList
		
	def normaliseArray(mins, array):
		newList = []
		for element in array:
			newList.append([element[1] - mins[1], element[0] - mins[0]])
		return newList

	print "Calculating size."
	maxPoints = [max(strip(points, 0)), max(strip(points, 1))]
	minPoints = [min(strip(points, 0)), min(strip(points, 1))]
	size = [maxPoints[1] - minPoints[1] + border[1], maxPoints[0] - minPoints[0] + border[0]]
	print "Normalising all data."
	points = normaliseArray(minPoints, points)

	print "Rendering."
	fig = plt.figure()
	matshow(samplemat(size, points, [border[0]/2, border[1]/2]), fignum=0, interpolation='bicubic', cmap=cm.gist_heat)
	print "Postprocessing."
	sys.stdout.flush()
	fig.canvas.set_window_title('Matrix-Heat : ' + shortFileName)
	saveName = "mh-" + shortFileName + ".png"
	savefig(saveName, dpi=720)
	print "Done, saved file as " + saveName
	
	#show()		#the interactive gui

if __name__ == "__main__":
	if len(sys.argv) == 2:
		main(sys.argv[1], 10)
	elif len(sys.argv) == 3:
		main(sys.argv[1], sys.argv[2])
	else:
		print """
== Matrix-Heat Quick Guide ==

Run: python matrix-heat.py string Relative_File [optional int Quality]
Relative_File: This is a path to the Frogatto statistics .cfg file to analyze.
Quality: How big of a brush to use. The bigger the brush, the longer it takes to draw a data point. Defaults to 10. Minimum is 5. It probaby doesn't need to be set larger than 25.

Notes:
This utility requires matplotlib to run, in addition to Python. You can download the stats files from http://wesnoth.org/files/dave/frogatto-stats/. You can change the palette by changing the cmap variable in the matshow function to these: http://www.scipy.org/Cookbook/Matplotlib/Show_colormaps. Uncomment 'show()' to enable the interactive GUI. For further details, write to http://www.frogatto.com/forum/index.php. Have a nice day. :)"""
		