# todo: Add better handling of non 72x72 sprites.
from PIL import Image, ImageDraw
import glob, os
from math import sqrt, ceil
from optparse import OptionParser, OptionValueError


border_color = (249,  48,  61)
fill_color   = (111, 109,  81)

def transform_tile(src):
    w = src.size[0] + 4
    h = src.size[1] + 4
    #if src.size[0] != 72 or src.size[1] != 72:
    #    return None
    #w = h = 76
    im = Image.new('RGBA', [w,h])
    draw = ImageDraw.Draw(im)
    draw.rectangle([(1,1),(w-2,h-2)], outline=border_color)
    im.paste(src, (2,2))
    return im

def create_tilemap(path_spec, dest_name):    
    file_list = glob.glob(path_spec)
    print file_list
    xoffs = 0
    yoffs = 0
    tx_list = []
    mw = 0
    mh = 0
    for infile in file_list:
        tx = transform_tile(Image.open(infile))
        mw = max(mw, tx.size[0])
        mh = max(mh, tx.size[1])
        if tx:
            tx_list.append(tx)
        else:
            print 'Ignored tile: %s. Size not 72x72' % infile
    # todo: if image is going to be over 1024x1024 make more than one.
    fl = int(ceil(sqrt(len(tx_list))))
    im = Image.new('RGBA', [fl * mw, fl * mh])
    for tx in tx_list:
        im.paste(tx, (xoffs*tx.size[0], yoffs*tx.size[1]))
        xoffs += 1
        if xoffs >= fl:
            yoffs += 1
            xoffs = 0
            
    im.save(dest_name, 'PNG')

#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\grass\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\grass.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\flat\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\flat.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\units\human-peasants\woodsman-idle*.png', r'C:\Projects\frogatto\modules\hex\images\units\woodsman-idle.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\mountains\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\mountains.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\water\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\water.png')

def main(options, args):
    create_tilemap(args[0], options.outfile)

def get_opts():
    usage = 'usage: %prog <options> <game_server:port>'
    parser = OptionParser(usage)
    parser.add_option('-v', '--verbose', action='store_true', default=False, dest='verbose')
    parser.add_option('-o', '--output', action='store', default='output.png', dest='outfile')
    return parser.parse_args()

if  __name__ == '__main__':
    options, args = get_opts()
    main(options, args)
