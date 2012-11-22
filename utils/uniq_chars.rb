#!/usr/bin/env ruby
# coding: UTF-8

# Read text from stdin, and output unique characters in it, as well
# as some additional characters needed in font textures

# all languages need these characters for missing translations and
# untranslatable strings
add = ['a'..'z', 'A'..'Z', '0'..'9',
       ' `~!@#$%^&*()-_=+[]{};:",.<>/?|→←↑↓'.chars].map(&:to_a).reduce(&:+) +

# these don't need glyphs, and will just mess up the font texture
minus = ["\n"]

STDOUT.write (ARGF.read.chars.to_a.uniq + add - minus).join
