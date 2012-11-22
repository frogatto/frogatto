#!/usr/bin/env python
""" -*- Mode: Python; tab-width: 4 -*-
    Recurses through the given path creating a text file containing the directory names.
    
    Author: Kristina Simpson <sweet.kristas@gmail.com>
"""

from optparse import OptionParser, OptionValueError
from os import walk, path

verbose = False
DEFAULT_OUTPUT_FILE = 'dirs.txt'

def create_dir_files( out_file_name, rootdir ):
    for root, subFolders, files in walk( rootdir ):
        if len( subFolders ) > 0:
            with open( path.join( root, out_file_name ), 'wb' ) as f:
                for folder in subFolders:
                    f.write( folder + '\n' )

def main():
    usage = 'usage: %prog [-o <name>] <root directory name>'
    parser = OptionParser( usage )
    parser.add_option( '-v', '--verbose', action='store_true', default=False, dest='verbose' )
    parser.add_option( '-o', '--output-file', action='store', default=DEFAULT_OUTPUT_FILE, dest='output_file', type='string' )
    ( options, args ) = parser.parse_args()
    if options.verbose: globals()[ 'verbose' ] = True
    if len( args ) < 1:
        parser.error( 'incorrect number of arguments' )
        return
    create_dir_files( options.output_file, args[0] )
        
if __name__ == '__main__':
    main()
