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
    boxID = 0
    for i in range(imgH):
        j = 0
        while j < imgW:
            if pixArray[i][j] == BOXRED:
                thisBox = findBox(pixArray, i, j)
                if thisBox is None: 
                    j += 1
                else:
                    boxes.append(thisBox)
                    j = thisBox[3] + 1 #continue the search *after* this box
                    pngX = thisBox[1] 
                    pngY = thisBox[0]
                    pngW = thisBox[3] - thisBox[1] + 1
                    pngH = thisBox[2] - thisBox[0] + 1
                    cropBox = "convert -crop " + pngW + "x" + pngH + "+" + \
                                pngX + "+" + pngY + " " + image + \ 
                                " make-gif-temp" + ("%03d" % boxID) + ".png"
                    boxID += 1
                    print cropBox
            else:
                j += 1
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
    boxFound = 0
    
    try:
        #see if we're on the first line/column of a box, by checking
        #if the pixels at offsets 1,0 and 0,1 are also red
        if array[line][col + 1] != BOXRED or array[line + 1][col] != BOXRED:
            return None
            
        #scan the pixels going down, starting at offset +1,+1 from the red pixel in the args
        while array[line2][col2] != BOXRED:
            line2 += 1
        while array[line + 1][col2] != BOXRED:
            col2 += 1
    except IndexError:
        return None
    
    #if line2 or col2 didn't change at all, no box was found
    if (line2 - line == 1) or (col2 - col == 1):
        return None
    return (line, col, line2, col2) 
    
def bash(arg):
    return subprocess.check_output(['bash','-c', arg])

if __name__ == "__main__":
	if len(sys.argv) == 2:
		main(sys.argv[1])
	else:
		pass
