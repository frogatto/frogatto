# Languages to generate font texture for
LANGUAGES = {
  'zh_CN' => 'wqy-zenhei.ttc',       # http://wenq.org/enindex.cgi
  'ja'    => 'TakaoPGothic.ttf'      # https://launchpad.net/takao-fonts
}

EXTRA_FML = Hash.new('').merge({
#  'ja' => "kerning=-1",
})

# Command template to generate glyph images for each font style
FONT_STYLES = {
  :dialog => <<-DIALOG, 
convert -background transparent -font '%{font}' -pointsize 16 \
        -fill '#f5f5f5' label:%{character} png32:%{glyph}
  DIALOG

  :outline => <<-'OUTLINE',
convert \( -background transparent -fill '#550b0b' -font '%{font}' label:%{character} \) \
        \( -background transparent -fill '#e9f9f9' -font '%{font}' label:%{character} \) \
        -background transparent \
        -page -1-1 -clone 0 \
        -page -1+1 -clone 0 \
        -page +1+1 -clone 0 \
        -page +1-1 -clone 0 \
        -page +0+0 -clone 1 \
        -delete 0,1 -layers merge +repage png32:%{glyph}
  OUTLINE
  
  :label => <<-'LABEL'
convert \( -background transparent -fill '#000000' -font '%{font}' label:%{character} \) \
        \( -background transparent -fill '#ffffff' -font '%{font}' label:%{character} \) \
        -background transparent \
        -page -0-1 -clone 0 \
        -page +0+0 -clone 1 \
        -delete 0,1 -layers merge +repage png32:%{glyph}
  LABEL
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

# Naming scheme of locale specific fonts.cfg
def fonts_cfg(language)
  "data/fonts.#{language}.cfg"
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
  task language => [
    fonts_cfg(language),
    *FONT_STYLES.keys.map do |style|
      [
      # glyphs_task(style, language), 
       font_texture(style, language), font_cfg_snippet(style, language)]
    end.flatten
  ]
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
      msggrep -v -N CHANGELOG -N MAC_APP_STORE_METADATA \
                 -N iOS_CHANGELOG -N iOS_APP_STORE_METADATA #{po_file} | \
      msgfilter --keep-header -o /dev/null tee -a #{msgstr_text}
      cat #{msgstr_text} | utils/strip_po_markup.sh \
                         | utils/uniq_chars.rb > #{character_list}
    SCRIPT
  end

  file fonts_cfg(language) do |t|
    File.open(t.name, 'w') do |file|
      file.write <<-EOF
[fonts]
	@include "data/number_font.cfg"
	@include "data/outline_font.#{language}.cfg"
	@include "data/label_font.#{language}.cfg"
	@include "data/dialog_font.#{language}.cfg"
[/fonts]
      EOF
    end
  end

  FONT_STYLES.each_pair do |style, command|
    work_path = work_path(style, language)
    directory work_path

    glyph_pattern = 
      Regexp.new(Regexp.escape(glyph_image(style, language, 'a')).sub(/\d+/, '.+'))
    rule glyph_pattern => work_path do |t|
      glyph = t.name
      character = character_with_codepoint(Integer(glyph[/(\d+)/, 1]))
      # escape for shell
      if character == "'"
        character = %q("'")
      else
        character = "'#{character}'"
      end
      sh(command % {:font => font, :character => character,
                    :glyph => glyph, :work_path => work_path})
    end

    glyphs = {}

    glyphs_task = glyphs_task(style, language) 
    task glyphs_task => character_list do 
      File.read(character_list).chars.each do |character|
        glyph = glyph_image(style, language, character)
        glyphs[glyph] = character
        Rake::Task[glyph].invoke
      end
    end

    font_texture = font_texture(style, language)
    desc "Generate #{language} #{style} font texture"
    file font_texture => [work_path, glyphs_task] do
      sh <<-COMMAND
        montage -label '' -background transparent -gravity SouthWest \
                -geometry '1x1+0+0<' #{glyphs.keys.join(' ')} png32:#{font_texture}
      COMMAND
      # For iOS optimization, the textures need to have even width and height
      (width, height) = image_size(font_texture).map {|length| length + length % 2}
     sh "convert #{font_texture} -gravity NorthWest -background transparent -extent #{width}x#{height} png32:#{font_texture}"
    end

    font_cfg_snippet = font_cfg_snippet(style, language)
    desc "Generate #{language} #{style} font data"
    file font_cfg_snippet => [work_path, font_texture, glyphs_task] do
      (texture_width, texture_height) = image_size(font_texture)

      widths = {}
      heights = {}

      glyphs.each_pair do |glyph, character|
        (widths[character], heights[character]) = image_size(glyph)
      end
      (grid_width, grid_height) = [widths, heights].map {|h| h.values.max}
      columns = texture_width / grid_width

      File.open(font_cfg_snippet, 'w') do |cfg|
        cfg.write <<-HEAD
          [font]
          id="#{FONT_CFG_IDS[style]}"
          texture=#{font_texture.sub %r'^images/', ''}
          pad=0
          #{EXTRA_FML[language]}
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

