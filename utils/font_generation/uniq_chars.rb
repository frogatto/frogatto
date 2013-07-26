#!/usr/bin/env ruby
# coding: UTF-8

# Read text from stdin, and output unique characters in it, as well
# as some additional characters needed in font textures

# all languages need these characters for missing translations and
# untranslatable strings
add = []

# these don't need glyphs, and/or cause the texture generation scripts
# to fail
minus = ["\n", "\\", '@']

STDOUT.write (ARGF.read.chars.to_a + add - minus).uniq.join
