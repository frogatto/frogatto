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
    imgW = int(sizeCheck[0])
    imgH = int(sizeCheck[1])
    
    pixArray = linesToArray(output, imgH, imgW)
    for i in range(imgH):
        for j in range(imgW):
            if pixArray[i][j] == BOXRED:
                print findBox(pixArray, i, j)
            #print i, j, pixArray[i][j]
    
def linesToArray(imageTxt, height, width):
    #newA = [[None] * width] * height #this fails in really unusual ways - all lines are the same as the last one
    newA = [[0 for x in range(width)] for y in range(height)] 
    #print imageTxt[32*320 + 2]
    #print imageTxt[32*320 + 3]
    #print imageTxt[33*320 + 3]
    for i in range(height):
        for j in range(width):
            newA[i][j] = simplify(imageTxt[width * i + j])
    return newA

def simplify(line):
    return '#' + line.split('#')[1][:6]

def findBox(array, line, col):
    lineInside = line + 1
    colInside = col + 1
    #scan the pixels going down, starting at offset +1,+1 from the red pixel in the args
    while array[lineInside][colInside] != BOXRED:
        lineInside += 1
    while array[line + 1][colInside] != BOXRED:
        colInside += 1
    return (line, col, lineInside, colInside) 
    

if __name__ == "__main__":
	if len(sys.argv) == 2:
		main(sys.argv[1])
	else:
		pass
