#!/usr/bin/env ruby
#
# Word wrap Frogatto po file msgstrs so that lines fit on the game screen according to
# glyph data defined in configuration files.
#
# Usage
#
#   utils/word_wrap_po.rb LANGUAGE
#
# LANGUAGE is the base name of the po file, for example zh_CN
#
# The script ensures that line breaks occur only at "legal" positions, e.g. never before
# closing punctuations. However it does not try to break at the most appealing positions.
#
# It ignores messages containing existing line breaks or {markup}, and messages
# from CHANGELOG, METADATA, and titlescreen files.
#
# Parts of the po file which the script "ignores" may still be modified, including
# msgids. However there should be only po metadata and formatting changes and the
# interpreted msgids and msgstrs remain the same.
#
# This script requires Ruby and the gems ffi-icu, get_pomo, and treetop.

require 'ffi-icu'
require 'get_pomo'
require 'treetop'
require 'fileutils'

MAX_LINE_WIDTH = 350
DEFAULT_KERNING = 2
LANGUAGE = ARGV[0]

CFG_FILE = "data/dialog_font.#{LANGUAGE}.cfg"
CFG_FILE.replace 'data/dialog_font.cfg' unless File.exist? CFG_FILE
PO_FILE = "po/#{LANGUAGE}.po"

unless File.exist? PO_FILE
  abort "#{PO_FILE} does not exist"
end

Treetop.load 'utils/fml_font_cfg.treetop'

parser = FrogattoFontCfgParser.new
result = parser.parse(File.read(CFG_FILE))
unless result
  puts parser.failure_reason
  abort "Failed to parse #{CFG_FILE}"
end
CHARACTER_WIDTHS = result.character_widths
KERNING = result.attributes.values['kerning'] || DEFAULT_KERNING

def word_wrap(text)
  line_breaker = ICU::BreakIterator.new(:line, LANGUAGE)
  line_breaker.text = text
  segments = line_breaker.each_cons(2).to_a
  lines = []
  until segments.empty?
    line = ''
    line_width = 0
    segments = segments.drop_while do |from, to|
      segment = text[from...to]
      width = segment.chars.map do |character|
        CHARACTER_WIDTHS[character].tap do |width|
          unless width
            abort "No glyph defined for #{character}, used in msgstr '#{text}'. If this language uses generated glyphs, try 'rake #{LANGUAGE}'"
          end
        end
      end.inject(&:+)
      line_width += width
      line_width <= MAX_LINE_WIDTH and
        begin
          line << segment
          line_width += KERNING
        end
    end
    lines << line.strip
  end
  if lines.length > 1
    puts "Word-wrapped:\n#{lines.join "\n"}"
  end
  lines.join "\\n"
end

po = GetPomo::PoFile.new
po.add_translations_from_text(File.read(PO_FILE))
po.translations.each do |translation|
  unless translation.msgid.empty? ||
         translation.msgstr.empty? || 
         translation.comment =~ /METADATA|CHANGELOG|titlescreen/ ||
         translation.msgstr =~ /[{}]|\\n/
    translation.msgstr = word_wrap(translation.msgstr.gsub('\"', '"')).gsub('"', '\"')
  end
end
File.open(PO_FILE, 'w') {|file| file.write po.to_text}

system "msgmerge #{PO_FILE} po/frogatto.pot -o #{PO_FILE}.part"
FileUtils.mv "#{PO_FILE}.part", PO_FILE

