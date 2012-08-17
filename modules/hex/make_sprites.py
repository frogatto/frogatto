# todo: Add better handling of non 72x72 sprites.
from PIL import Image, ImageDraw
import glob, os
from math import sqrt, ceil

border_color = (249,  48,  61)
fill_color   = (111, 109,  81)

def transform_tile(src):
    #w = src.size[0] + 8
    #h = src.size[1] + 8
    if src.size[0] != 72 or src.size[1] != 72:
        return None
    w = h = 76
    im = Image.new('RGBA', [w,h], fill_color)
    draw = ImageDraw.Draw(im)
    draw.rectangle([(1,1),(w-2,h-2)], outline=border_color)
    im.paste(src, (2,2), src)
    return im

def create_tilemap(path_spec, dest_name):    
    file_list = glob.glob(path_spec)
    xoffs = 0
    yoffs = 0
    tx_list = []
    for infile in file_list:
        tx = transform_tile(Image.open(infile))
        if tx:
            tx_list.append(tx)
        else:
            print 'Ignored tile: %s. Size not 72x72' % infile
            
    fl = int(ceil(sqrt(len(tx_list))))
    im = Image.new('RGBA', [fl * 76, fl * 76], fill_color)
    for tx in tx_list:
        im.paste(tx, (xoffs*76, yoffs*76), tx)
        xoffs += 1
        if xoffs >= fl:
            yoffs += 1
            xoffs = 0
            
    im.save(dest_name, 'PNG')

#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\grass\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\grass.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\flat\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\flat.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\units\human-peasants\woodsman-idle*.png', r'C:\Projects\frogatto\modules\hex\images\units\woodsman-idle.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\mountains\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\mountains.png')
create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\water\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\water.png')
