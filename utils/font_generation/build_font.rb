#!/usr/bin/env ruby

require './image_size.rb'
images_gui_path = File.expand_path('../../images/gui')

help = <<"HELP"
  This script builds a font cfg and texture from a list of glyphs.

Usage

  #$0 GLYPH-DIRECTORY FONT-NAME FONT-ID [PADDING [KERNING]]

  FONT-NAME
    The path of the .cfg file to create. The basename of this path also
    determines the basename of the texture to create, which is saved
    in #{images_gui_path}.

  FONT-ID, PADDING, KERNING
    Values of the id, padding and kerning attributes in the created .cfg file.

  GLYPH-DIRECTORY
    Directory containing glyph .png images, the basenames of which are
    codepoints of their characters.
HELP

# Quote a character in FML
def fml_quote(character)
  case character
  when '"'
    %("\\"")
  when  "'"  
    %("'")
  else
    %("#{character}")
  end
end

abort help unless ARGV.length == 3

glyph_directory = ARGV[0]
font_cfg_path = ARGV[1]
font_id, padding, kerning = ARGV[2..4]

texture_name = "#{File.basename(font_cfg_path, '.cfg')}.png"
texture_path = File.join images_gui_path, texture_name
characters = []
glyphs = Dir.new(glyph_directory).select do |name|
  codepoint = Integer(File.basename(name, '.png')) rescue nil
  if codepoint
    characters << codepoint.chr(Encoding::UTF_8)
  end
end
glyphs.map! {|basename| "#{glyph_directory}/#{basename}"}

abort help if glyphs.empty?

puts "Saving #{texture_path}"
# Montage the font texture
system 'montage', '-background', 'transparent', '-gravity', 'West', *glyphs,
       '-geometry', '1x1+0+0<', '+set', 'label', "png32:#{texture_path}"

# For iOS optimization, the textures need to have even width and height
(texture_width, texture_height) = image_size(texture_path).map {|length| length + length % 2}
system 'convert', texture_path, '-gravity', 'NorthWest', '-background', 'transparent',
       '-extent', "#{texture_width}x#{texture_height}", "png32:#{texture_path}"

# Generate .cfg file
glyph_width = {}
glyph_height = {}

characters.each_with_index do |character, index|
  (glyph_width[character], glyph_height[character]) = image_size(glyphs[index])
end

(grid_width, grid_height) = [glyph_width, glyph_height].map {|h| h.values.max}
columns = texture_width / grid_width

File.open(font_cfg_path, 'w') do |cfg|
  cfg.write <<-HEAD
    {
      kerning: #{kerning || 1},
      id: "#{font_id}",
      texture: "gui/#{texture_name}",
      pad: 0,
      chars: [
  HEAD

  characters.each_slice(columns).each_with_index do |row, row_index|
    row.each_with_index do |character, column_index|
      left = column_index * grid_width
      top = row_index * grid_height
      right = left + glyph_width[character] - 1
      bottom = top + grid_height - 1
      cfg.write <<-CHARS
        {
          chars: #{fml_quote(character)},
          rect: [#{left},#{top},#{right},#{bottom}]
        },
      CHARS
    end
  end
  cfg.write '] }'
end

