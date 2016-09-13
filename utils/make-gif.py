#!/usr/bin/env python
# -*- coding: utf-8 -*-
import subprocess, sys
BOXRED = "#F9303D"
BGBROWN = "#6F6D51"

def main(image):
    #global BOXRED, BGBROWN
    bashCommand = "convert " + image + " -depth 8 txt:-"
    output = subprocess.check_output(['bash','-c', bashCommand])
    output = "".join(output)
    output = output.split('\n')
    output = output[1:] #discard the header line: "#Image"


    sizeCheck = "identify -format \"%w,%h\" " + image
    sizeCheck = subprocess.check_output(['bash','-c', sizeCheck])
    sizeCheck = sizeCheck.split(',')
    imgH = int(sizeCheck[0])
    imgW = int(sizeCheck[1])
    
    pixArray = linesToArray(output, imgW, imgH)
    
def linesToArray(imageTxt, width, height):
    simplify = lambda x: '#' + x.split('#')[1][:6]
    newA = [[None] * height] * width
    for i in range(width):
        for j in range(height):
            newA[i][j] = simplify(imageTxt[width * i + j])

if __name__ == "__main__":
	if len(sys.argv) == 2:
		main(sys.argv[1])
	else:
		pass
