# todo: Add better handling of non 72x72 sprites.
from PIL import Image, ImageDraw
import glob, os
from math import sqrt, ceil
from optparse import OptionParser, OptionValueError
import json


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

def common_prefix(strings):
    """ Find the longest string that is a prefix of all the strings.
    """
    if not strings: return ''
    prefix = strings[0]
    for s in strings:
        if len(s) < len(prefix):
            prefix = prefix[:len(s)]
        if not prefix: return ''
        for i in range(len(prefix)):
            if prefix[i] != s[i]:
                prefix = prefix[:i]
                break
    return prefix
    
def create_tilemap(path_specs, dest_name, transitions):
    file_list = []
    for path in path_specs:
        file_list += glob.glob(path)
    print '%d files in list' % (len(file_list))
    print file_list
    common_file_name = common_prefix(file_list)
    
    txs = {}
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
            tx_list.append((infile, tx))
        else:
            print 'Ignored tile: %s. Size not 72x72' % infile
    # todo: if image is going to be over 1024x1024 make more than one.
    fl = int(ceil(sqrt(len(tx_list))))
    im = Image.new('RGBA', [fl * mw, fl * mh])
    for fn, tx in tx_list:
        im.paste(tx, (xoffs*tx.size[0], yoffs*tx.size[1]))
        prefix = fn.replace(common_file_name, '').replace('.png', '')
        if len(prefix) > 0 and prefix[0] == '-': prefix = prefix[1:]
        if len(prefix) > 0 and prefix[-1] == '-': prefix = prefix[:-1]
        x1,y1,x2,y2 = xoffs*tx.size[0]+2, yoffs*tx.size[1]+2, (xoffs+1)*tx.size[0]-3,(yoffs+1)*tx.size[1]-3
        txs[prefix] = {"rect":[x1,y1,x2,y2], "image":dest_name}
        print '"%s":{"image":"%s", "rect":[%d,%d,%d,%d]},' % (prefix,dest_name,x1,y1,x2,y2)
        xoffs += 1
        if xoffs >= fl:
            yoffs += 1
            xoffs = 0
            
    im.save(dest_name, 'PNG')
    if transitions:
        # create a blank terrain document describing the transitions
        with open(transitions, 'wb') as f:
            f.write(json.dumps(txs, separators=(',', ':')))

#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\grass\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\grass.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\flat\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\flat.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\units\human-peasants\woodsman-idle*.png', r'C:\Projects\frogatto\modules\hex\images\units\woodsman-idle.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\mountains\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\mountains.png')
#create_tilemap(r'C:\Projects\wesnoth-1.10.3\data\core\images\terrain\water\*.png', r'C:\Projects\frogatto\modules\hex\images\tiles\water.png')

def main(options, args):
    create_tilemap(args, options.outfile, options.transitions)

def get_opts():
    usage = 'usage: %prog <options> <game_server:port>'
    parser = OptionParser(usage)
    parser.add_option('-v', '--verbose', action='store_true', default=False, dest='verbose')
    parser.add_option('-o', '--output', action='store', default='output.png', dest='outfile')
    parser.add_option('-t', '--transitions', action='store', default=None, dest='transitions')
    return parser.parse_args()

if  __name__ == '__main__':
    options, args = get_opts()
    main(options, args)
