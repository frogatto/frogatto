# Languages to generate font texture for
LANGUAGES = {
  'zh_CN' => 'myuppygb-medium.ttf',
  'ja'    => 'ipag.ttf'
}

# Command template to generate glyph images for each font style
FONT_STYLES = {
  :dialog =>  "convert -background transparent -font '%{font}' -pointsize 16 \
                       -fill '#f5f5f5' label:'%{character}' png32:%{glyph}",

  :outline => "convert -size 300x300 xc:transparent \
                       -font '%{font}' -pointsize 12 \
                       -fill '#550b0b' -annotate +51+49 '%{character}' \
                                       -annotate +49+51 '%{character}' \
                                       -annotate +51+51 '%{character}' \
                                       -annotate +49+49 '%{character}' \
                       -fill '#e9f9f9' -annotate +50+50 '%{character}' \
                       -trim png32:%{glyph}",

  :label =>   "convert -size 300x300 xc:transparent \
                       -font '%{font}' -pointsize 12 \
                       -fill '#000000' -annotate +50+51 '%{character}' \
                       -fill '#ffffff' -annotate +50+50 '%{character}' \
                       -trim png32:%{glyph}"
}

FONT_CFG_IDS = {
  :dialog  => 'default',
  :label   => 'door_label',
  :outline => 'white_outline'
}

# Naming scheme of font textures
def font_texture(style, language)
  "images/gui/#{style}_font.#{language}.png"
end

# Naming scheme of fonts.cfg snippets
def font_cfg_snippet(style, language)
  "data/#{style}_font.#{language}.cfg"
end

# Naming scheme of character lists
def character_list(language)
  "tmp/fonts/#{language}/characters.txt"
end

# Naming scheme of po files
def po_file(language)
  "po/#{language}.po"
end

# Naming scheme of font generation working path
def work_path(style, language)
  "tmp/fonts/#{language}/#{style}"
end

# Naming scheme of glyphs
def glyph_image(style, language, character)
  File.join work_path(style, language), "#{codepoint_of(character)}.png"
end

# Naming scheme of font tasks
def font_task(style, language)
  "font:#{style}:#{language}"
end

# Naming scheme of glyph tasks
def glyphs_task(style, language)
  "glyphs:#{style}:#{language}"
end

# Width and height of an image
def image_size(path)
  `file #{path}` =~ /(\d+)\s*x\s*(\d+)/
  [$1, $2].map(&method(:Integer))
end

# Unicode codepoint of a character
def codepoint_of(character)
  character.unpack('U*').first
end

# Character with given unicode codepoint
def character_with_codepoint(codepoint)
  [codepoint].pack('U*')
end

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

task :default => :fonts

desc 'Generate all font textures and fonts.cfg snippets'
task :fonts => (LANGUAGES.keys.map do |language|
  desc "Generate #{language} font textures and fonts.cfg snippets"
  task language => (
    FONT_STYLES.keys.map do |style|
      [glyphs_task(style, language), font_task(style, language)]
    end.flatten
  )
end)

# We need to generate a texture and snippet for each pair of (language, font style)
LANGUAGES.each_pair do |language, font|
  character_list = character_list(language)
  msgstr_text = "#{character_list}.raw"
  po_file = po_file(language)
  language_work_path = work_path('', language)
  directory language_work_path
  desc "Generate #{language} character list"
  file character_list => [po_file, language_work_path] do
    sh <<-SCRIPT
      cat /dev/null > #{msgstr_text}
      msgfilter --keep-header -o /dev/null tee -a #{msgstr_text} < #{po_file}
      cat #{msgstr_text} | utils/strip_po_markup.sh \
                         | utils/uniq_chars.rb > #{character_list}
    SCRIPT
  end

  FONT_STYLES.each_pair do |style, command|
    glyph_pattern = 
      Regexp.new(Regexp.escape(glyph_image(style, language, 'a')).sub(/\d+/, '.+'))
    rule glyph_pattern do |t|
      glyph = t.name
      character = character_with_codepoint(Integer(glyph[/(\d+)/, 1]))
      sh(command % {:font => font, :character => character, :glyph => glyph})
    end

    work_path = work_path(style, language)
    directory work_path

    glyphs = {}

    glyphs_task = glyphs_task(style, language) 
    task glyphs_task => character_list do 
      File.read(character_list).chars.each do |character|
        glyph = glyph_image(style, language, character)
        glyphs[glyph] = character
        task font_task(style, language) => glyph
      end
    end

    desc "Generate #{language} #{style} font texture and fonts.cfg snippet"
    task font_task(style, language) => [work_path, glyphs_task] do
      font_texture = font_texture(style, language)
      sh <<-COMMAND
        montage -label '' -background transparent -gravity SouthWest \
                -geometry '1x1+0+0<' #{glyphs.keys.join(' ')} png32:#{font_texture}
      COMMAND
      (texture_width, texture_height) = image_size(font_texture)

      widths = {}
      heights = {}

      glyphs.each_pair do |glyph, character|
        (widths[character], heights[character]) = image_size(glyph)
      end
      (grid_width, grid_height) = [widths, heights].map {|h| h.values.max}
      columns = texture_width / grid_width

      font_cfg_snippet = font_cfg_snippet(style, language)
      File.open(font_cfg_snippet, 'w') do |cfg|
        cfg.write <<-HEAD
          [font]
          id="#{FONT_CFG_IDS[style]}"
          texture=#{font_texture.sub %r'^images/', ''}
          pad=0
        HEAD
         
        glyphs.values.each_slice(columns).each_with_index do |characters, row|
          characters.each_with_index do |character, column|
            left = column * grid_width
            top = row * grid_height
            right = left + widths[character] - 1
            bottom = top + grid_width - 1
            cfg.write <<-CHARS
              [chars]
              chars=#{fml_quote(character)}
              rect=#{left},#{top},#{right},#{bottom}
              [/chars]
            CHARS
          end
        end
        cfg.write '[/font]'
      end
    end
  end
end

