#!/usr/bin/env python
# -*- coding: utf-8 -*-
import subprocess, sys
BOXRED = "#F9303D"
BGBROWN = "#6F6D51"

def main(image):
    #global BOXRED, BGBROWN
    bashCommand = "convert " + image + " -depth 8 txt:-"
    output = bash(bashCommand)
    output = "".join(output)
    output = output.split('\n')
    output = output[1:] #discard the header line: "#Image"

    sizeCheck = "identify -format \"%w,%h\" " + image
    sizeCheck = bash(sizeCheck)
    sizeCheck = sizeCheck.split(',')
    imgW = int(sizeCheck[0])
    imgH = int(sizeCheck[1])
    
    
    pixArray = linesToArray(output, imgH, imgW)
    boxes = []
    for i in range(imgH):
        for j in range(imgW):
            if pixArray[i][j] == BOXRED:
                thisBox = findBox(pixArray, i, j)
                if thisBox is not None: 
                    boxes.append(thisBox)
                    j = thisBox[3] + 1 #continue the search *after* this box
                    print thisBox
            #print i, j, pixArray[i][j]
    
def linesToArray(imageTxt, height, width):
    #newA = [[None] * width] * height #this fails in really unusual ways - all lines are the same as the last one
    newA = [[0 for x in range(width)] for y in range(height)] 
    for i in range(height):
        for j in range(width):
            newA[i][j] = simplify(imageTxt[width * i + j])
    return newA

def simplify(line):
    return '#' + line.split('#')[1][:6]

def findBox(array, line, col):
    line2 = line + 1
    col2 = col + 1
    #see if we're on the first line of a box, by checking if the pixel to the right is also red
    if 
        return None
    #scan the pixels going down, starting at offset +1,+1 from the red pixel in the args
    try:
        while array[line2][col2] != BOXRED:
            line2 += 1
        while array[line + 1][col2] != BOXRED:
            col2 += 1
    except IndexError:
        return None
    return (line, col, line2, col2) 
    
def bash(arg):
    return subprocess.check_output(['bash','-c', arg])

if __name__ == "__main__":
	if len(sys.argv) == 2:
		main(sys.argv[1])
	else:
		pass
