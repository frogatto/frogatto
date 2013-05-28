#!/usr/bin/env ruby

require 'psych'

tmpdir = 'tmp/language_name_font'

Psych.load(File.read('../../../../data/languages.cfg')).each_pair do |code, name|
  font = "../../../../data/label_font.#{code}.cfg"
  if !File.exist? font
    font = "../../../../data/label_font.cfg"
  end

  IO.popen(['./extract_glyphs.rb', tmpdir, font], 'w') do |io|
    io.write name
  end
end

system "./build_font.rb #{tmpdir} ../../../../data/language_name_font.cfg language_names"
