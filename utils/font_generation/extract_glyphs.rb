#!/usr/bin/env ruby

images_paths = ['anura/images/', '../../images/'].map(&File.method(:expand_path))

help = <<"HELP"
  This script extracts glyphs from Frogatto fonts

USAGE

  #$0 OUTPUT-DIR INPUT-FONT.cfg.. < CHARACTERS

  or

  echo 'abcdefg' | #$0 OUTPUT-DIR INPUT-FONT.cfg..

  INPUT-FONT.cfg..
    The script will extract glyphs from the fonts defined in these .cfg files.

  CHARACTERS
    The script reads a list of characters to be extracted from stdin. If
    a character is not defined in the first input font, the script looks
    for it in the second intput font, and so forth.

  OUTPUT-DIR
    The script will store glyphs under , where the base name of
    each .png file is the codepoint of its character.

EXAMPLE FONT.cfg

  {
    texture: "texture.png" # path relative to #{images_paths.join(' or ')}
    pad: 1,
    chars: [
      {
        chars: "a",
        rect: [2,1,8,20], # left, top, right, bottom
      }
      {
        chars: "bc",
        width: 6, # left + pad + width, top, right + pad + width, bottom
      }
    ]
  }
HELP

require 'psych'
require 'fileutils'
require 'set'

abort help unless ARGV.length >= 2

output_dir = ARGV[0]
input_fonts = ARGV[1..-1]
output_characters = $stdin.read.chars.to_set

FileUtils.mkdir_p output_dir

input_fonts.each_with_index do |font_file, index|
  font = Psych.load IO.read(font_file)
  texture = images_paths.map {|path| File.join(path, font['texture'])}.find(&File.method(:exist?))
  padding = font['pad'] || 2
  characters = {}

  left = 1
  top = 1
  width = 0
  height = 0

  font['chars'].each do |group|
    if group['rect']
      left, top, right, bottom = group['rect']
      height, width = bottom - top + 1, right - left + 1

    elsif group['width']
      width = group['width']
    end

    group['chars'].chars.each do |character|
      if output_characters.include? character
        glyph_file = File.join(output_dir, "#{character.codepoints.first}.png")
        puts "extracting glyph of '#{character}' from #{texture} at #{width}x#{height}+#{left}+#{top} to #{glyph_file}"
        system 'convert', '-extract', "#{width}x#{height}+#{left}+#{top}",
                          texture, glyph_file
        output_characters.delete character
      end

      # advance to the right for the next character
      left = left + width + padding
    end
  end
end

