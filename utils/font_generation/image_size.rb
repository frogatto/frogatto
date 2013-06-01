# Width and height of an image
def image_size(path)
  `file #{path}` =~ /(\d+)\s*x\s*(\d+)/
  [$1, $2].map(&method(:Integer))
end
