from PIL import Image
from optparse import OptionParser, OptionValueError
from math import log

def next_pot(cnt): return pow(2,int(log(cnt,2))+1)

border_color = (249,  48,  61)
fill_color   = (111, 109,  81)

def create_palette_map(in_file, out_file, cs):
    im = Image.open(in_file)
    colors_list = im.getcolors(1024)
    assert colors_list, 'List of colors exceeds 1024, consider refactoring'
    shifted_colors = []
    for cnt, color in colors_list:
        if color[0:3] != border_color and color[0:3] != fill_color:
            #shifted_colors.append((color,((cs[0]+color[0])%256,(cs[1]+color[1])%256,(cs[2]+color[2])%256,color[3])))
            shifted_colors.append((color,((cs[0]+color[0]),(cs[1]+color[1]),(cs[2]+color[2]),color[3])))
    # height of image is next power-of-two of the number of colors
    h = next_pot(len(colors_list))
    
    om = Image.new('RGBA', [2, h], fill_color)
    hh = 0
    for p0,p1 in shifted_colors:
        om.putpixel([0,hh],p0)
        om.putpixel([1,hh],p1)
        hh += 1
    om.save(out_file, 'PNG')
    
def main(options, args):
    create_palette_map(args[0], options.outfile, (options.red, options.green, options.blue))

def get_opts():
    usage = 'usage: %prog <options> <game_server:port>'
    parser = OptionParser(usage)
    parser.add_option('-v', '--verbose', action='store_true', default=False, dest='verbose')
    parser.add_option('-o', '--output', action='store', default='output.png', dest='outfile')
    # XXX: Add a custom decoder for --rgb=x,y,z.
    parser.add_option('-r', '--red', action='store', type='int', default=0, dest='red')
    parser.add_option('-g', '--green', action='store', type='int', default=0, dest='green')
    parser.add_option('-b', '--blue', action='store', type='int', default=0, dest='blue')
    return parser.parse_args()

if  __name__ == '__main__':
    options, args = get_opts()
    main(options, args)
